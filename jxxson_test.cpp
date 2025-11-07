//
// MIT License
// 
// Copyright (c) 2025 Henry Du
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <iostream>
#include <fstream>
#include <chrono>

#include "jxxson.hpp"

int main(int argc, char** argv) {
    std::ifstream in_json("test.json");
    std::ofstream out_json("test-out.json");
    auto start = std::chrono::high_resolution_clock::now();
    jxxson::document_tree<> tree{16777216};
    jxxson::fill_tree(tree, std::istreambuf_iterator<char>(in_json), std::istreambuf_iterator<char>());
    auto middle = std::chrono::high_resolution_clock::now();
    tree.format_to(std::ostreambuf_iterator<char>(out_json));
    auto stop = std::chrono::high_resolution_clock::now();

    std::cout << std::format("Input  time costs: {}\n", std::chrono::duration<double>(middle - start));
    std::cout << std::format("Output time costs: {}\n", std::chrono::duration<double>(stop   - middle));
}