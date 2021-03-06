// ----------------------------------------------------------------------------
// -                        Open3D: www.open-3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018, Intel Visual Computing Lab
// Copyright (c) 2018, Open3D community
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include <tinyfiledialogs/tinyfiledialogs.h>
#include <Open3D/Core/Core.h>

void PrintHelp()
{
    using namespace open3d;
    PrintInfo("Usage :\n");
    PrintInfo("    > TestFileDialog [save|load]\n");
}

int32_t main(int32_t argc, char *argv[])
{
    using namespace open3d;
    if (argc == 1) {
        PrintHelp();
        return 0;
    }
    std::string option(argv[1]);
    char const *pattern = "*.*";
    if (option == "load") {
        char const *str = tinyfd_openFileDialog(
                "Find a file to load", "", 0, NULL, NULL, 1);
        PrintInfo("%s\n", str);
    } else if (option == "save") {
        char const *str = tinyfd_saveFileDialog("Find a file to save", "", 1,
                &pattern, NULL);
        PrintInfo("%s\n", str);
    }
    return 0;
}
