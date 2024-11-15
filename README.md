# semiopen_interval encoding

A semi-open interval map associates ranges [start_included, end_excluded) to values.

For example, querying this map
```
m: 
[-1, 10) -> one
[10, 11) -> two
[13, 15) -> three

```

will return these values:


```
m[-2] == no value
m[0]  == one
m[9] == one
m[10] == two
m[12] == no value
m[13] == three
m[15] == no value
m[16] == no value
```

In this encoding, no value is represented by a value passed to the constructor.

This repo is the implementation of the assign(map, range, value) method, with some unit testing and a fuzz tester built with clang libfuzzer.