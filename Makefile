OUTFILE ?= out
MOVESLB ?= 10
gen: build
	./generator 10 30 5 $(OUTFILE) $(MOVESLB)

gen_easy: build
	mkdir -p easy
	./generator 10 10 5 easy 10

gen_medium: build
	mkdir -p medium
	./generator 10 20 5 medium 20

gen_hard: build
	mkdir -p hard
	./generator 10 30 5 hard 30

.PHONY: build
build: gen.cpp
	g++ -std=c++17 -Wshadow -Wall -Wextra -Wformat=2 -Wconversion -Wfatal-errors -Ofast gen.cpp -o generator
