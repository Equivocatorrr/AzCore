# Requirements and Design Choices for Asset Systems

## Basic Requirements
- Read asset files from disk
- Decode/decompress files into more immediately useful form
- Make data available for use

## Wants
- On-the-fly asset streaming
- Parallel pipelining from single-stream disk reading to enough parallel workers to decode files as quickly as possible
- Moving resources to GPU for use individually (also on-the-fly)
- Get handle to an asset as soon as requested to be made available for use later
- Ability to poll asset state (either simple "ready" flag or enum for stages)
- Only keep in RAM as long as necessary, moving to GPU and removing from Host
- Priority-based queue

## Stages
- Requested
- Loading from disk
- Queued for decoding (probably especially transient)
- Decoding
- Uploading (only valid for GPU resources)
- Ready

## Kinds of Assets
### General
- Images
- Fonts
- Sounds
- Sound Streams (Somewhat antiquated with how much memory is available these days. Maybe just decode piece by piece, but load whole file into RAM. With high-quality ogg compression 3 minute music files are in the 2-4 MB range.)
### Engine-Specific
- 3D Models
- Rigged Animations
- Shaders
- Scripts
- Game Worlds

## API
A generic API for managing these stages for any number of user-defined asset types may be a good idea.

The generic system would be responsible for:
- Managing priority queue for loading from disk
- Loading the file data from disk in one thread
- Managing separate worker threads for decoding purposes (min 1, max hardware concurrency - 2)

The engine would be responsible for:
- Providing decoder functionality and managing the destination memory in a thread-safe way
- Uploading data to GPU
- Telling the generic system when it's okay to clean up the file memory (usually right after decoding)