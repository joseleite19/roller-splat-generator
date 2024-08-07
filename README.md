# roller-splat-generator
A generator for Roller Splat puzzle(https://apps.apple.com/us/app/roller-splat/id1448852425)

All the logic is currently inside the `gen.cpp` file.

## Build

```
make build
```
Should compile the code using C++ 17.

## Usage
```
./generator <number of rows> <number of columns> <number of boards to generate> <output path> <minimum number of moves to solve each board>
```

An example would be

```
./generator 5 10 2 . 10
```

The generated boards will be created with sequencial order numbering(`1.txt`, `2.txt`, ...) starting with the largest number already used - so new executions do not overwrite past files.

## Shorter commands
```
make gen_easy
```
This command will try to create five boards of an easier dificult on the `easy` folder.

```
make gen_medium
```
This command will try to create five boards of an medium dificult  on the `medium` folder.

```
make gen_hard
```
This command will try to create five boards of an harder dificult on the `hard` folder. This can take a few seconds to execute.

# Further improvements
- Usage of threads to generate boards in parallel
- Usage of heuristics to generate boards faster

- Implement LRU cache
- Add limits for search space - depth or number of moves, total moves tried
- Checks if there is solution only after a few moves(~5) and not at every move
- Re-implement the brute force checker without recursion

