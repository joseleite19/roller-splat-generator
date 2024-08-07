#include <bits/stdc++.h>
#include <filesystem>

using namespace std;

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

struct Board {
	size_t n, m;
	vector<vector<bool>> can_pass; // boolean tells whether we can pass by a cell or not
	vector<vector<int>> visited_count; // keeps the number of times we passed on each cell
	using FlatBoard = bitset<32 * 32>; // allows 32 x 32 boards
	FlatBoard flat_board; // flatten board used for fast comparisons, keeps track of which cells we have passed on
	int x, y;
	static constexpr int dx[4] = {1, 0, -1, 0}; // helper for moving in the four cardinal directions
	static constexpr int dy[4] = {0, 1, 0, -1};
	vector<tuple<int, int, int>> moves; //stack of last moves performed, used for rollbacks of last move on bruteforce

	Board(Board &&other) noexcept : n(other.n), m(other.m), can_pass(std::move(other.can_pass)), visited_count(std::move(other.visited_count)), flat_board(std::move(other.flat_board)), x(other.x), y(other.y), moves(std::move(other.moves)) {
	}
	Board(const Board &other) noexcept : n(other.n), m(other.m), can_pass(other.can_pass), visited_count(other.visited_count), flat_board(other.flat_board), x(other.x), y(other.y), moves(other.moves) {
	}
	void operator=(const Board &other) {
		n = other.n, m = other.m;
		can_pass = other.can_pass;
		visited_count = other.visited_count;
		flat_board = other.flat_board;
		x = other.x, y = other.y;
		moves = other.moves;
	}
	Board(const vector<string> &str_board, int px, int py) : n(str_board.size()), m(str_board[0].size()), can_pass(n, vector<bool>(m)), visited_count(n, vector<int>(m, 0)), x(px), y(py) {

		flat_board.set();
		for(size_t i = 0; i < str_board.size(); i++) {
			for(size_t j = 0; j < str_board[i].size(); j++) {
				change_block(int(i), int(j), str_board[i][j]);
			}
		}
		flat_board[x * m + y] = 1;
		visited_count[x][y] = 1;
	}

	void change_block(int i, int j, char c) {
		if(c == '#') {
			can_pass[i][j] = false;
			flat_board[i * m + j] = 1;
		} else {
			can_pass[i][j] = true;
			flat_board[i * m + j] = 0;
		}
	}

	bool can_use(int i, int j) const {
		if(i < 0 or i >= (int)can_pass.size()) return false;
		if(j < 0 or j >= (int)can_pass[i].size()) return false;
		return can_pass[i][j];
	}
	void move(int dir) {
		moves.push_back({x, y, dir});

		while(can_use(x + dx[dir], y + dy[dir])) {
			x += dx[dir];
			y += dy[dir];
			flat_board[x * m + y] = 1;
			visited_count[x][y]++;
		}
	}

	void rollback() {
		auto [newx, newy, dir] = moves.back();
		moves.pop_back();

		while(x != newx or y != newy) {
			if(!--visited_count[x][y]) {
				flat_board[x * m + y] = 0;
			}

			x -= dx[dir];
			y -= dy[dir];
		}
	}
	void rollback_all() {
		while(!moves.empty()) rollback();
	}
	
	bool won() const {
		return flat_board.all();
	}

	struct HashFunction {
		size_t operator()(const Board &board) const {
			auto ans = std::hash<FlatBoard>()(board.flat_board);
			ans ^= std::hash<size_t>()(board.x * board.m + board.y) << 1;
			return ans;
		}
	};
	bool operator==(const Board &b) const {
		return flat_board == b.flat_board && x == b.x and y == b.y;
	}

	void print(std::ostream &out) const {
		for(size_t i = 0; i < n; i++) {
			for(size_t j = 0; j < m; j++) {
				if((int)i == x and (int)j == y) {
					out << 'x';
				} else if(!can_pass[i][j]) {
					out << '#';
				} else if(visited_count[i][j]) { // used only on debugging - usefull to show path of a solution printing after every move
					out << '-';
				} else {
					out << '.';
				}
			}
			out << '\n';
		}
		out << '\n';
	}
};

using HashSet = unordered_set<Board, Board::HashFunction>;
using HashMap = unordered_map<Board, int, Board::HashFunction>;

struct LRU {
	unordered_map<Board, list<Board>::iterator, Board::HashFunction> hash_map;
	list<Board> order;

	const size_t size_lim = 500'000;
	// size_t bloom = 0;

	bool insert(const Board &b) {
		//bloom |= Board::HashFunction{}(b);
		auto [it, inserted] = hash_map.insert({b, order.end()});
		if(inserted) {
			order.push_back(b);
			it->second = prev(order.end());

			if(order.size() > size_lim) {
				hash_map.erase(order.front());
				order.pop_front();
			}

			return true;
		}

		order.splice(order.end(), order, it->second);
		return false;
	}
	inline bool exists(const Board &b) {
		//size_t hashing = Board::HashFunction{}(b);
		//if((bloom & hashing) != hashing) return false;
		auto it = hash_map.find(b);

		if(it == hash_map.end()) {
			return false;
		}

		order.splice(order.end(), order, it->second);
		return true;
	}
};

bool can_win_iterative(Board &b, size_t depth) {
	static LRU lose_states; // only one LRU cache across all calls, it saves all states that are impossible to win
	LRU visiting_states;

	vector<pair<array<int, 4>, int>> stack;
	array<int, 4> moves = {0, 1, 2, 3};
	shuffle(moves.begin(), moves.end(), rng);

	stack.push_back({moves, 0});

	int search_limit = 1'000'000;
	while(!stack.empty()) {
		if(--search_limit < 0) {
			b.rollback_all();
			return false;
		}
		
		if(stack.back().second > 0) {
			b.rollback();
		}
		
		if(stack.back().second >= 4) {
			lose_states.insert(b);
			// visiting_states.erase(b);
			stack.pop_back();
			continue;
		}
		
		if(b.won()) {
			b.rollback_all();
			return true;
		}
		
		if(stack.size() > depth) {
			stack.pop_back();
			continue;
		}
		
		if(stack.back().second == 0 and (!visiting_states.insert(b) or lose_states.exists(b))) {
			stack.pop_back();
			continue;
		}

		b.move(stack.back().first[ stack.back().second++ ]);
		shuffle(moves.begin(), moves.end(), rng);
		stack.push_back({moves, 0});
	}

	return false;
}

bool can_win(Board &b, HashSet &visiting_states, int depth) {
	static LRU lose_states; // only one LRU cache across all calls, it saves all states that are impossible to win
	if(b.won()) return true;
	
	if(depth <= 0 or !visiting_states.insert(b).second or lose_states.exists(b)) return false;
	
	array<int, 4> moves = {0, 1, 2, 3};
	shuffle(moves.begin(), moves.end(), rng);
	for(int i : moves) {
		b.move(i);
		bool ans = can_win(b, visiting_states, depth - 1);
		b.rollback();
		if(ans) return true;
	}
	lose_states.insert(b);
	return false;
}
long double can_win_total_time = 0;
bool can_win(Board &b) {
	// HashSet visiting_states;
	const int max_depth = 100;
	// int ans = can_win(b, visiting_states, max_depth);
	// cout << "<<<<<<<<<<< " << ans << " " << can_win_iterative(b, size_t(max_depth)) << endl;
	// assert(ans == can_win_iterative(b, size_t(max_depth)));
	// return ans;

	auto start = clock();

	bool ans = can_win_iterative(b, size_t(max_depth));

	can_win_total_time += (long double)(clock() - start) / CLOCKS_PER_SEC; 

	return ans;
}

void shortest_win(Board &b, HashSet &visited_states, int &shortest_solution, int cur_moves) {
	if(cur_moves >= shortest_solution) return; // the current path will never result in a better solution
	if(b.won()) {
		shortest_solution = cur_moves;
		return;
	}
	if(!visited_states.insert(b).second) return;

	for(int i : {0, 1, 2, 3}) {
		b.move(i);
		shortest_win(b, visited_states, shortest_solution, cur_moves + 1);
		b.rollback();
	}
}

int shortest_win(Board &b) {
	int shortest_solution = INT_MAX;
	HashSet visited_states;
	shortest_win(b, visited_states, shortest_solution, 0);
	return shortest_solution;
}

int rand(int l, int r) {
	return uniform_int_distribution<int>(l, r)(rng);
}

Board build(int n, int m) {

	vector<string> str_board(n, string(m, '#'));

	int x = rand(0, n - 1);
	int y = rand(0, m - 1);

	int dir = rand(0, 3);

	int curx = x, cury = y;
	str_board[x][y] = '.';
	Board board(str_board, x, y);

	auto in_bounds = [n, m](int i, int j) {
		return 0 <= i and i < n and 0 <= j and j < m;
	};

	vector<pair<int, bool>> buffer;
	int buffer_cnt = 0;
	const int it_limit = 2 * n * m + rand(0, min(200, n * m));
	for(int it = 0; it < it_limit or buffer_cnt > 0; it++) { // after 2 * n * m iteractions, there is a 2% chance of stopping
		cerr << it << endl;
		if(rand(0, 10) == 0) {
			dir = rand(0, 3);
		}

		while(!in_bounds(curx + Board::dx[dir], cury + Board::dy[dir])) {
			dir = rand(0, 3);
		}

		curx += Board::dx[dir], cury += Board::dy[dir];

		buffer.push_back({dir, str_board[curx][cury] == '#'});
		if(str_board[curx][cury] == '#') {
			board.change_block(curx, cury, '.');
			str_board[curx][cury] = '.';

			if(++buffer_cnt >= 5) {
				if(!can_win(board)) {
					// if not possible to win, revert last few changes and change directions
					while(!buffer.empty()) {
						auto [ddir, should_change] = buffer.back(); buffer.pop_back();
						if(should_change) {
							board.change_block(curx, cury, '#');
							str_board[curx][cury] = '#';
						}
						curx -= Board::dx[ddir], cury -= Board::dy[ddir];
						dir = ddir;
					}
					dir = rand(0, 3);
				}
				buffer.clear();
				buffer_cnt = 0;
			}
			/*if(!can_win(board)) {
				board.change_block(curx, cury, '#');
				str_board[curx][cury] = '#';
				curx -= Board::dx[dir], cury -= Board::dy[dir];
				int old_dir = dir;
				do { dir = rand(0, 3); } while(dir == old_dir);
			}*/
		}
	}

	return board;
}

vector<Board> gen(int n, int m, size_t lim, int moves_lowerbound) {

	long double shortest_wins_time = 0;
	long double build_board_time = 0;
	int interactions = 0;
	vector<Board> boards;
	while(boards.size() < lim) {
		interactions++;
		auto start_building_time = clock();
		Board board = build(n, m);
		build_board_time += (long double)(clock() - start_building_time) / CLOCKS_PER_SEC;
		
		auto start_shortest_time = clock();
		int moves = shortest_win(board);
		shortest_wins_time += (long double)(clock() - start_shortest_time) / CLOCKS_PER_SEC;

		if(moves >= moves_lowerbound) {
			boards.push_back(board);
		}
	}

	cerr << "Number of interactions to generate all boards: " << interactions << endl;
	cerr << "Total time building boards: " << build_board_time << endl;
	cerr << "Total time checking if there is a winning solution for a board: " << can_win_total_time << endl;
	cerr << "Total time computing shortest wins: " << shortest_wins_time << endl << endl;

	return boards;
}

// ./gen n m cnt_boards outpath solution_moves_lowerbound
int main(int argc, char **argv) {

	if(argc < 6) {
		cerr << "USAGE ./generate <n> <m> <cnt_boards> <output_path> <moves_lowerbound>" << endl;
		exit(1);
	}

	int n = atoi(argv[1]);
	int m = atoi(argv[2]);
	int cnt_boards = atoi(argv[3]);
	string out_path = argv[4];
	int moves_lowerbound = atoi(argv[5]);

	if(n * m > 32 * 32) {
		cerr << "N * M should be at most 32 * 32" << endl;
		exit(1);
	}

	int output_id = 1;

	for (const auto & entry : filesystem::directory_iterator(out_path)) {
		try {
			output_id = max(output_id, 1 + stoi(entry.path().filename()));
		}
		catch (const invalid_argument& ia) {

		}
	}

	for(auto board : gen(n, m, cnt_boards, moves_lowerbound)) {
		cerr << "Number of moves to win: " << shortest_win(board) << endl;
		string output_file = out_path + "/" + to_string(output_id) + ".txt";
		cerr << "Saving file " << output_file << endl;
		std::ofstream outfile(output_file);
		board.print(outfile); // save the board on the output file
		board.print(cerr); // show the board on the cerr

		output_id++;
	}
}
