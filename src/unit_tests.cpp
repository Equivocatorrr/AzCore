/*
    File: unit_tests.cpp
    Author: Philip Haynes
    Does exactly as advertized.
*/

#include "log_stream.hpp"
#include "memory.hpp"

void Print(vec3 v, io::logStream& cout) {
    cout << "{";
    for (u32 i = 0; i < 3; i++) {
        if (v[i] >= 0.0)
            cout << " ";
        if (v[i] < 10.0)
            cout << " ";
        if (v[i] < 100.0)
            cout << " ";
        cout << v[i];
        if (i != 2)
            cout << ", ";
    }
    cout << "}";
}

void Print(mat3 m, io::logStream& cout) {
    cout << "[" << std::fixed << std::setprecision(3);
    Print(m.Row1(), cout);
    cout << "\n ";
    Print(m.Row2(), cout);
    cout << "\n ";
    Print(m.Row3(), cout);
    cout << "]" << std::endl;
}

void Print(vec4 v, io::logStream& cout) {
    cout << "{";
    for (u32 i = 0; i < 4; i++) {
        if (v[i] >= 0.0)
            cout << " ";
        if (v[i] < 10.0)
            cout << " ";
        if (v[i] < 100.0)
            cout << " ";
        cout << v[i];
        if (i != 3)
            cout << ", ";
    }
    cout << "}";
}

void Print(mat4 m, io::logStream& cout) {
    cout << "[" << std::fixed << std::setprecision(3);
    Print(m.Row1(), cout);
    cout << "\n ";
    Print(m.Row2(), cout);
    cout << "\n ";
    Print(m.Row3(), cout);
    cout << "\n ";
    Print(m.Row4(), cout);
    cout << "]" << std::endl;
}

void UnitTestMat3(io::logStream& cout) {
    cout << "Unit testing mat3\n";
    cout << "identity = \n";
    Print(mat3(), cout);
    cout << "\nRow1 = ";
    Print(mat3().Row1(), cout);
    cout << "\nRow2 = ";
    Print(mat3().Row2(), cout);
    cout << "\nRow3 = ";
    Print(mat3().Row3(), cout);
    cout << "\nCol1 = ";
    Print(mat3().Col1(), cout);
    cout << "\nCol2 = ";
    Print(mat3().Col2(), cout);
    cout << "\nCol3 = ";
    Print(mat3().Col3(), cout);
    cout << "\nRotated pi/4 around x-axis:\n";
    Print(mat3::RotationBasic(halfpi/2, Axis::X), cout);
    Print(mat3::Rotation(halfpi/2, {1.0, 0.0, 0.0}), cout);
    cout << "\nRotated pi/4 around y-axis:\n";
    Print(mat3::RotationBasic(halfpi/2, Axis::Y), cout);
    Print(mat3::Rotation(halfpi/2, {0.0, 1.0, 0.0}), cout);
    cout << "\nRotated pi/4 around z-axis:\n";
    Print(mat3::RotationBasic(halfpi/2, Axis::Z), cout);
    Print(mat3::Rotation(halfpi/2, {0.0, 0.0, 1.0}), cout);
    cout << "\nScaled by {2.0, 2.0, 2.0}:\n";
    Print(mat3::Scaler({2.0, 2.0, 2.0}), cout);
    cout << "\nRotated by pi about {0.5, 0.5, 0.0}:\n";
    Print(mat3::Rotation(pi, {0.5, 0.5, 0.0}), cout);
    cout << "\nRotated by pi about {0.5, 0.5, 0.5}:\n";
    Print(mat3::Rotation(pi, {0.5, 0.5, 0.5}), cout);
    mat3 m = mat3(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    cout << "New mat3 = \n";
    Print(m, cout);
    cout << "Transpose:\n";
    Print(m.Transpose(), cout);
}

void UnitTestMat4(io::logStream& cout) {
    cout << "Unit testing mat4\n";
    cout << "identity = \n";
    Print(mat4(), cout);
    cout << "\nRow1 = ";
    Print(mat4().Row1(), cout);
    cout << "\nRow2 = ";
    Print(mat4().Row2(), cout);
    cout << "\nRow3 = ";
    Print(mat4().Row3(), cout);
    cout << "\nRow4 = ";
    Print(mat4().Row4(), cout);
    cout << "\nCol1 = ";
    Print(mat4().Col1(), cout);
    cout << "\nCol2 = ";
    Print(mat4().Col2(), cout);
    cout << "\nCol3 = ";
    Print(mat4().Col3(), cout);
    cout << "\nCol4 = ";
    Print(mat4().Col4(), cout);
    cout << "\nRotated pi/4 around xy-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::XY), cout);
    cout << "\nRotated pi/4 around xz-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::XZ), cout);
    cout << "\nRotated pi/4 around xw-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::XW), cout);
    cout << "\nRotated pi/4 around yz-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::YZ), cout);
    cout << "\nRotated pi/4 around yw-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::YW), cout);
    cout << "\nRotated pi/4 around zw-plane:\n";
    Print(mat4::RotationBasic(halfpi/2, Plane::ZW), cout);
    cout << "\nScaled by {2.0, 2.0, 2.0, 2.0}:\n";
    Print(mat4::Scaler({2.0, 2.0, 2.0, 2.0}), cout);
    cout << "\nRotated by pi about {0.5, 0.5, 0.0}:\n";
    Print(mat4::Rotation(pi, {0.5, 0.5, 0.0}), cout);
    cout << "\nRotated by pi about {0.5, 0.5, 0.5}:\n";
    Print(mat4::Rotation(pi, {0.5, 0.5, 0.5}), cout);
    mat4 m = mat4( 1.0,  2.0,  3.0,  4.0,
                   5.0,  6.0,  7.0,  8.0,
                   9.0, 10.0, 11.0, 12.0,
                  13.0, 14.0, 15.0, 16.0);
    cout << "New mat4 = \n";
    Print(m, cout);
    cout << "Transpose:\n";
    Print(m.Transpose(), cout);
}

void UnitTestComplex(io::logStream& cout) {
    cout << "Unit testing complex numbers\n";
    complex c, z;
    for (i32 y = -40; y <= 40; y++) {
        for (i32 x = -70; x <= 50; x++) {
            c = z = complex(f32(x)/40.0, f32(y)/20.0);
            const char val[] = " `*+%";
            u32 its = 0;
            for (; its < 14; its++) {
                z = pow(z,4.0) + c;
                if (abs(z) > 2.0)
                    break;
            }
            cout << val[its/3];
        }
        cout << "\n";
    }
    complex a(2, pi);
    a = exp(a);
    cout << "exp(2 + pi*i) = (" << a.real << " + " << a.imag << "i)\n";
    a = log(a);
    cout << "log of previous value = (" << a.real << " + " << a.imag << "i)\n";
    cout << std::endl;
}

void UnitTestQuat(io::logStream& cout) {
    cout << "Unit testing quaternions\n";
    quat q;
    cout << "Rotation(pi/4, {1.0, 0.0, 0.0}):\n";
    q = quat::Rotation(pi/4.0, {1.0, 0.0, 0.0});
    Print(q.wxyz, cout);
    cout << std::endl;
    Print(q.ToMat3(), cout);
    cout << "Rotation(pi/4, {0.0, 1.0, 0.0}):\n";
    q = quat::Rotation(pi/4.0, {0.0, 1.0, 0.0});
    Print(q.wxyz, cout);
    cout << std::endl;
    Print(q.ToMat3(), cout);
    cout << "Rotation(pi/4, {0.0, 0.0, 1.0}):\n";
    q = quat::Rotation(pi/4.0, {0.0, 0.0, 1.0});
    Print(q.wxyz, cout);
    cout << std::endl;
    Print(q.ToMat3(), cout);
    cout << "Multiplying two pi/2 rotations on different axes:\nq1: ";
    mat3 m = mat3::Rotation(pi/2.0, {1.0, 0.0, 0.0});
    q = quat::Rotation(pi/2.0, {1.0, 0.0, 0.0});
    Print(q.wxyz, cout);
    cout << "\nToMat3():\n";
    Print(q.ToMat3(), cout);
    cout << "Control Matrix:\n";
    Print(m, cout);
    cout << "q2: ";
    mat3 m2 = mat3::Rotation(pi/2.0, {0.0, 1.0, 0.0});
    quat q2 = quat::Rotation(pi/2.0, {0.0, 1.0, 0.0});
    Print(q2.wxyz, cout);
    cout << "\nToMat3():\n";
    Print(q2.ToMat3(), cout);
    cout << "Control Matrix:\n";
    Print(m2, cout);
    cout << "q1*q2: ";
    quat q3 = q * q2;
    Print(q3.wxyz, cout);
    cout << "\nToMat3():\n";
    Print(q3.ToMat3(), cout);
    cout << "Control Matrix:\n";
    Print(m * m2, cout);
    cout << "q2*q1: ";
    q3 = q2 * q;
    Print(q3.wxyz, cout);
    cout << "\nToMat3():\n";
    Print(q3.ToMat3(), cout);
    cout << "Control Matrix:\n";
    Print(m2 * m, cout);
    quat a(pi, 0, -1, 0);
    a = exp(a);
    cout << "exp(pi - j) = (" << a.w << " + " << a.x << "i + " << a.y << "j + " << a.z << "k)\n";
    a = log(a);
    cout << "log of previous value = (" << a.w << " + " << a.x << "i + " << a.y << "j + " << a.z << "k)\n";
    cout << std::endl;
}

void UnitTestSlerp(io::logStream& cout) {
    cout << "Unit testing slerp:\n";
    quat a(0.0, 1.0, 0.0, 0.0);
    quat b(0.0, 0.0, 1.0, 0.0);
    cout << "With a = ";
    Print(a.wxyz, cout);
    cout << " and b = ";
    Print(b.wxyz, cout);
    cout << "\nslerp(a,b,-1.0) = ";
    Print(slerp(a,b,-1.0).wxyz, cout);
    cout << "\nslerp(a,b,0.0) = ";
    Print(slerp(a,b,0.0).wxyz, cout);
    cout << "\nslerp(a,b,1/3) = ";
    Print(slerp(a,b,1.0/3.0).wxyz, cout);
    cout << "\nslerp(a,b,0.5) = ";
    Print(slerp(a,b,0.5).wxyz, cout);
    cout << "\nslerp(a,b,1.0) = ";
    Print(slerp(a,b,1.0).wxyz, cout);
    cout << "\nslerp(a,b,2.0) = ";
    Print(slerp(a,b,2.0).wxyz, cout);
    cout << std::endl;
}

void UnitTestRNG(RandomNumberGenerator& rng, io::logStream& cout) {
    cout << "Unit testing RandomNumberGenerator\n";
    {
        u32 count[100] = {0};
        for (u32 i = 0; i < 100000; i++) {
            count[rng.Generate()%100]++;
        }
        cout << std::dec << "After 100000 numbers generated, 0-100 has the following counts:\n{";
        for (u32 i = 0; i < 100; i++) {
            if (count[i] < 10)
                cout << " ";
            if (count[i] < 100)
                cout << " ";
            if (count[i] < 1000)
                cout << " ";
            cout << count[i];
            if (i != 99) {
                cout << ", ";
                if (i%10 == 9)
                    cout << "\n ";
            }
        }
        cout << "}" << std::endl;
    }
    UniquePtr<u16[]> count(new u16[1000000]);
    for (u32 i = 0; i < 1000000; i++) {
        count[i] = 0;
    }
    cout << "After 10,000,000 numbers generated, 0-1,000,000 missed ";
    for (u32 i = 0; i < 10000000; i++) {
        count[rng.Generate()%1000000]++;
    }
    u32 total = 0;
    for (u32 i = 0; i < 1000000; i++) {
        if (count[i] == 0) {
            total++;
        }
    }
    cout << total << " indices." << std::endl;
}

void Print(List<i32> list, io::logStream& cout) {
    cout << "{";
    bool first = true;
    for (i32 i = 0; i < list.size; i++) {
        if (!first) {
            cout << ", ";
        }
        cout << list[i];
        first = false;
    }
    cout << "}";
}

void PrintRef(const List<i32>& list, io::logStream& cout) {
    cout << "{";
    bool first = true;
    for (i32 i = 0; i < list.size; i++) {
        if (!first) {
            cout << ", ";
        }
        cout << list[i];
        first = false;
    }
    cout << "}";
}

void UnitTestList(io::logStream& cout) {
    cout << "Unit testing List<i32>\n";
    List<i32> list = {1, 2, 3, 4};
    cout << "list.size = " << list.size << std::endl;
    cout << "list = ";
    Print(list, cout);
    cout << " by value, ";
    PrintRef(list, cout);
    cout << " by reference";
    cout << "\nafter inserting 5 at index 0: ";
    list.Insert(0, 5);
    Print(list, cout);
    cout << " by value, ";
    PrintRef(list, cout);
    cout << " by reference";
    cout << "\nlist.size = " << list.size << std::endl;
    cout << "\nafter inserting 6 at index 3: ";
    list.Insert(3, 6);
    Print(list, cout);
    cout << " by value, ";
    PrintRef(list, cout);
    cout << " by reference";
    cout << "\nlist.size = " << list.size << std::endl;
    cout << "\nafter erasing index 2: ";
    list.Erase(2);
    Print(list, cout);
    cout << " by value, ";
    PrintRef(list, cout);
    cout << " by reference";
    cout << "\nlist.size = " << list.size << std::endl;
    cout << "\nafter erasing index 0: ";
    list.Erase(0);
    Print(list, cout);
    cout << " by value, ";
    PrintRef(list, cout);
    cout << " by reference";
    cout << "\nlist.size = " << list.size << std::endl;
    cout << "\nafter resizing to 8: ";
    list.Resize(8);
    Print(list, cout);
    cout << " by value, ";
    PrintRef(list, cout);
    cout << " by reference";
    cout << "\nlist.size = " << list.size << std::endl;
    cout << "\nafter resizing to 3: ";
    list.Resize(3);
    Print(list, cout);
    cout << " by value, ";
    PrintRef(list, cout);
    cout << " by reference";
    cout << "\nlist.size = " << list.size << std::endl;
}

template<typename T>
void PrintArray(const Array<T>& array, const char* name, io::logStream& cout) {
    cout << name << " = {";
    for (i32 i = 0; i < array.size; i++) {
        cout << array[i];
        if (i != array.size-1) {
            cout << ", ";
        }
    }
    cout << "}" << std::endl;
}

void UnitTestArrayAndString(io::logStream& cout) {
    Array<u32> test1 = {
        1, 2, 3, 4, 5, 6, 7, 8, 9
    };
    Array<String> test2 = {
        "There once was a man who hated cheese.",
        "He was also an absolute dick.",
        "You should probably stay away from him."
    };
    PrintArray(test1, "test1", cout);
    PrintArray(test2, "test2", cout);
    cout << "Adding values to the end of both Arrays..." << std::endl;
    test1 += 10;
    test2 += "I think everything should be okay anyways.";
    PrintArray(test1, "test1", cout);
    PrintArray(test2, "test2", cout);
    cout << "Adding values to the beginning of both Arrays..." << std::endl;
    test1.Insert(0,0);
    test2.Insert(0,"Once upon a time,");
    PrintArray(test1, "test1", cout);
    PrintArray(test2, "test2", cout);
    cout << "Adding values to the middle of both Arrays..." << std::endl;
    test1.Insert(7,67);
    test2.Insert(3,"And he likes to hurt people.");
    PrintArray(test1, "test1", cout);
    PrintArray(test2, "test2", cout);
    cout << "Erasing values from the middle of both Arrays..." << std::endl;
    test1.Erase(6);
    test1.Erase(7);
    test2.Erase(5);
    PrintArray(test1, "test1", cout);
    PrintArray(test2, "test2", cout);
    cout << "Reversing both Arrays..." << std::endl;
    test1.Reverse();
    test2.Reverse();
    PrintArray(test1, "test1", cout);
    PrintArray(test2, "test2", cout);
    cout << "Appending both Arrays with other Arrays..." << std::endl;
    test1 += {11, 12, 13, 14, 15};
    test2 += {"What was I talking about?", "This is getting pretty weird!"};
    PrintArray(test1, "test1", cout);
    PrintArray(test2, "test2", cout);
    cout << "Resizing both Arrays to 5..." << std::endl;
    test1.Resize(5);
    test2.Resize(5);
    PrintArray(test1, "test1", cout);
    PrintArray(test2, "test2", cout);
    cout << "Using copy constructors..." << std::endl;
    Array<u32> test3(test1);
    Array<String> test4(test2);
    PrintArray(test3, "test3", cout);
    PrintArray(test4, "test4", cout);
    cout << "Using copy assignment..." << std::endl;
    Array<u32> test5;
    test5 = test1;
    Array<String> test6;
    test6 = test2;
    PrintArray(test5, "test5", cout);
    PrintArray(test6, "test6", cout);
    cout << "Printing them all added together with +..." << std::endl;
    Array<u32> test7 = test1 + test3 + test5;
    Array<String> test8 = test2 + test4 + test6;
    PrintArray(test7, "test7", cout);
    PrintArray(test8, "test8", cout);
}
