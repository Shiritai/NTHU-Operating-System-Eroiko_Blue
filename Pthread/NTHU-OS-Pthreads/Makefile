CXX = g++
CXXFLAGS = -static -std=c++11 -O3
LDFLAGS = -pthread
TARGETS = main reader_test producer_test consumer_test writer_test ts_queue_test
DEPS = transformer.cpp

.PHONY: all
all: $(TARGETS)

.PHONY: docker-build
docker-build:
	docker-compose run --rm build

.PHONY: clean
clean:
	rm -f $(TARGETS)

%: %.cpp $(DEPS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^

make:
	scl enable devtoolset-8 'make'

trans0:
	./scripts/auto_gen_transformer --input ./tests/00_spec.json --output transformer.cpp

trans1:
	./scripts/auto_gen_transformer --input ./tests/01_spec.json --output transformer.cpp

run_test0:
	./main 200 ./tests/00.in ./tests/00.out
	./scripts/verify --output ./tests/00.out --answer ./tests/00.ans

rebuild_and_run_test0:
	make trans0
	make rebuild
	make run_test0

run_test1:
	./main 4000 ./tests/01.in ./tests/01.out
	./scripts/verify --output ./tests/01.out --answer ./tests/01.ans

rebuild_and_run_test1:
	make trans1
	make rebuild
	make run_test1

rebuild:
	make clean
	make make

test_all:
	make rebuild_and_run_test0
	make rebuild_and_run_test1

exp:
	make trans1
	make rebuild
	./main 4000 ./tests/01.in ./tests/01.out expe
	./scripts/verify --output ./tests/01.out --answer ./tests/01.ans

exp_msg:
	make trans1
	make rebuild
	./main 4000 ./tests/01.in ./tests/01.out expe msg
	./scripts/verify --output ./tests/01.out --answer ./tests/01.ans
