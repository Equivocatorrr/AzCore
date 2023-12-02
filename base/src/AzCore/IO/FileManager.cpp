/*
	File: FileManager.cpp
	Author: Philip Haynes
*/

#include "FileManager.hpp"
#include "../QuickSort.hpp"
#include "Log.hpp"

namespace AzCore::io {

void FileManager::Init(i32 numDecodeWorkers) {
	cout.PrintLnTrace("Calling " __FUNCTION__);
	AzAssert(!initted, "Calling FileManager::Init when it's already been initted");
	if (numDecodeWorkers < 1) {
		numDecodeWorkers = max((i32)Thread::HardwareConcurrency() - 2, 2);
	}
	close = false;
	filesInPipeline = 0;
	threadsDecode.Resize(numDecodeWorkers);
	for (i32 i = 0; i < threadsDecode.size; i++) {
		threadsDecode[i] = Thread(DecodeProc, this);
	}
	availableDecoders = numDecodeWorkers;
	threadDisk = Thread(DiskProc, this);
	initted = true;
}

void FileManager::Deinit() {
	cout.PrintLnTrace("Calling " __FUNCTION__);
	AzAssert(initted, "Calling FileManager::Deinit when it hasn't been initted");
	close = true;
	condRequested.WakeAll();
	threadDisk.Join();
	condToDecode.WakeAll();
	for (i32 i = 0; i < threadsDecode.size; i++) {
		threadsDecode[i].Join();
	}
	initted = false;
}

void FileManager::WaitUntilDone() {
	cout.PrintLnTrace("Calling " __FUNCTION__);
	mutexPipeline.Lock();
	while (!close && filesInPipeline > 0) {
		condPipeline.Wait(mutexPipeline);
	}
	mutexPipeline.Unlock();
}

static void SortFiles(Array<File*> &array) {
	QuickSort(array, [](File *lhs, File *rhs){ return rhs->priority < lhs->priority; });
}

File* FileManager::RequestFile(String filepath, i32 priority, File::fp_Decoder decoder, Any userdata) {
	cout.PrintLnTrace("Calling " __FUNCTION__ " for \"", filepath, "\"");
	AzAssert(initted, "Calling FileManager::RequestFile when it hasn't been initted");
	mutexFiles.Lock();
	File &result = files.ValueOf(filepath, File{filepath, {}, std::move(userdata), decoder, priority, File::Stage::REQUESTED});
	mutexFiles.Unlock();
	if (result.stage == File::Stage::REQUESTED || result.stage == File::Stage::DISCARDED) {
		mutexPipeline.Lock();
			filesInPipeline++;
		mutexPipeline.Unlock();
		mutexRequested.Lock();
			filesRequested.Append(&result);
			SortFiles(filesRequested);
		mutexRequested.Unlock();
		condRequested.WakeOne();
	}
	return &result;
}

File* FileManager::RequestDecode(Array<char> &&buffer, String filepath, i32 priority, File::fp_Decoder decoder, Any userdata) {
	cout.PrintLnTrace("Calling " __FUNCTION__ " for \"", filepath, "\"");
	AzAssert(initted, "Calling FileManager::RequestFile when it hasn't been initted");
	AzAssert(decoder != nullptr, "Why are you calling RequestDecode without any decoder???");
	mutexFiles.Lock();
	File &result = files.ValueOf(filepath, File{filepath, std::move(buffer), std::move(userdata), decoder, priority, File::Stage::READY_FOR_DECODE});
	mutexFiles.Unlock();
	if (result.stage == File::Stage::READY_FOR_DECODE || result.stage == File::Stage::DISCARDED) {
		mutexPipeline.Lock();
			filesInPipeline++;
		mutexPipeline.Unlock();
		mutexToDecode.Lock();
			filesToDecode.Append(&result);
			SortFiles(filesToDecode);
		mutexToDecode.Unlock();
		condToDecode.WakeOne();
	}
	return &result;
}

static void DeclareFileCompleteInPipeline(FileManager *manager) {
	manager->mutexPipeline.Lock();
		manager->filesInPipeline--;
	manager->mutexPipeline.Unlock();
	manager->condPipeline.WakeAll();
}

void FileManager::DiskProc(FileManager *manager) {
	manager->mutexRequested.Lock();
	while (!manager->close) {
		// NOTE: We have to check because it's possible for all requests to be placed before the thread is finished spinning up, in which case the condition variable signal would be lost and we would never start working
		while (manager->filesRequested.size == 0) {
			manager->condRequested.Wait(manager->mutexRequested);
			if (manager->close) goto full_break;
		}
		
		while (!manager->close && manager->filesRequested.size > 0) {
			File *requested = manager->filesRequested.Back();
			manager->filesRequested.size--;
			manager->mutexRequested.Unlock(); // Allow more requests to be made while we load the file
			
			requested->stage = File::Stage::LOADING;
			bool found = false;
			for (const String &dir : manager->searchDirectories) {
				requested->data = FileContents(dir + requested->filepath, true);
				if (requested->data.size != 0) {
					found = true;
					break;
				}
			}
			if (found) {
				if (requested->decoder) {
					requested->stage = File::Stage::READY_FOR_DECODE;
					manager->mutexToDecode.Lock();
						manager->filesToDecode.Append(requested);
						SortFiles(manager->filesToDecode);
					manager->mutexToDecode.Unlock();
					// A file is ready for decode, alert a decoder
					manager->condToDecode.WakeOne();
				} else {
					requested->stage = File::Stage::READY;
					DeclareFileCompleteInPipeline(manager);
				}
			} else {
				if (manager->warnFileNotFound) {
					cerr.Lock().PrintLn("File not found: \"", requested->filepath, "\"").Unlock();
				}
				requested->stage = File::Stage::FILE_NOT_FOUND;
				DeclareFileCompleteInPipeline(manager);
			}
			manager->mutexRequested.Lock();
		}
	}
full_break:
	manager->mutexRequested.Unlock();
}

void FileManager::DecodeProc(FileManager *manager) {
	manager->mutexToDecode.Lock();
	while (!manager->close) {
		while (manager->filesToDecode.size == 0) {
			manager->condToDecode.Wait(manager->mutexToDecode);
			if (manager->close) goto full_break;
		}
		
		while (!manager->close && manager->filesToDecode.size > 0) {
			File *toDecode = manager->filesToDecode.Back();
			manager->filesToDecode.size--;
			manager->availableDecoders--;
			manager->mutexToDecode.Unlock(); // Allow more decode requests to be made and other decode threads to run while we decode this one
			
			AzAssert(toDecode->decoder != nullptr, "Somehow we got a file with no decoder to the decoding step");
			toDecode->stage = File::Stage::DECODING;
			if (toDecode->decoder(toDecode, toDecode->userdata)) {
				toDecode->stage = File::Stage::READY;
			} else {
				toDecode->data.Clear();
				toDecode->stage = File::Stage::DISCARDED;
			}
			DeclareFileCompleteInPipeline(manager);
			
			manager->mutexToDecode.Lock();
			manager->availableDecoders++;
		}
	}
full_break:
	manager->mutexToDecode.Unlock();
}

} // namespace AzCore::io