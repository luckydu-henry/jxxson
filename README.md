# jxxson
jxxson (Jackson) is a rapid C++20 json library

## BUILD
Just add `jxxson.hpp` to your include directroy and have a C++20 compatible compiler, and you are ready to go.
This library has passed `g++-14` and `clang++-19` test on linux and `MSVC17` test on windows, so feel free to use it on any platform or compiler.

## INTRODUCTION
Consider this library as the brother of [xxmlxx](https://github.com/luckydu-henry/xxmlxx) which is a XML library also developed by me.
Their core techniques are very similar, both very light and fast. Since JSON itself is also simpler than XML so `jxxson`'s source code is also much more smaller than `xxmlxx` **(~700 lines of code)**

They both use *Breadth First Vector Tree (BFVT)* as their container.
BFVT is extremely cache friendly and iterate friendly, nodes are stored in a sorted std::vector according to their parent node index, which makes all nodes in a 'breadth first search' sequence. Thus, **the space complexity of this tree's breadth-first-iteraton is O(1)** (just a normal vector iteraton). What's more, you can search children nodes with std::upper_bound (binary-search) which makes searching children in a large tree much more faster, and the **time complexity of this tree's 'depth-first-iteraton' is O(nlogn)** but in many cases is smaller then that.

This technique also brought some issues and that's 'slow insertion', since nodes are stored inside a vector, insert nodes frequently can be very slow (check input and output time comparison, it's scary), I'm still working on batch-insertion (insert many nodes at a time to reduce insertion count). It's also good to have contributors.

## FEATURES
As I said, now, reading tree from file or string is slow, but, writing tree to string or file can be very fast.
A quick start to use IO functionalaties is:
```c++
std::ifstream in_json("test.json");
std::ofstream out_json("test-out.json");
jxxson::document_tree<> tree{16777216}; // Allocate pool according to your json's size, basically greater than your json's line count is enough.
jxxson::fill_tree(tree, std::istreambuf_iterator<char>(in_json), std::istreambuf_iterator<char>());
tree.format_to(std::ostreambuf_iterator<char>(out_json));
```
And you can also use a very convenient API to access and modify your json
```c++
auto nodes = tree["scenes"][0]["nodes"];
nodes[0]->value() = 1;
nodes[1]->value() = 2;
```
If you want to `double` or `long long` for some particular large number json, you can:
```c++
jxxson::document_tree<long long, double> larger_tree;
```
By the way, this test file is [sponza.gltf]() (WebGL Transition Format), a typical json file that can be used for benchmark.