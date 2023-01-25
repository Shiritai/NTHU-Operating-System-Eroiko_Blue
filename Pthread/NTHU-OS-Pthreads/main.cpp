#include <assert.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <unordered_set>
#include <sys/wait.h>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"

#define READER_QUEUE_SIZE 200
#define WORKER_QUEUE_SIZE 200
#define WRITER_QUEUE_SIZE 4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 1000000

/** 
 * Conduct a single run with assigned parameters
 * @param r_qs     reader queue size
 * @param wk_qs    worker queue size
 * @param wt_qs    writer queue size
 * @param cs_h_th  consumer controller high threshold size
 * @param cs_l_th  consumer controller low threshold size
 * @param cs_per   check period of consumer controller
 * @param in       input file name
 * @param out      output file name
 * @param lines    numbers of lines in "in" file
 */
void one_run(
	int r_qs, int wk_qs, int wt_qs, 
	int cs_h_th, int cs_l_th, int cs_per, 
	std::string & in, std::string & out, 
	int lines) 
{
	// prepare queues
	auto input_queue = new TSQueue<Item *>(r_qs),
		worker_queue = new TSQueue<Item *>(wk_qs),
		writer_queue = new TSQueue<Item *>(wt_qs);

	// init and start reader thread
	auto reader = new Reader(lines, in, input_queue);
	reader->start();

	// init and start writer thread
	auto writer = new Writer(lines, out, writer_queue);
	writer->start();

	// prepare transformer
	auto transformer = new Transformer();

	// producer vector, init with four identical producers and start all of them
	std::vector<Producer> pv(4, Producer(input_queue, worker_queue, transformer));
	for (auto &p: pv) p.start(); // start all producers
	
	// init and start consumer controller thread
	auto ctr = new ConsumerController(
		worker_queue, writer_queue, transformer, 
		cs_per, wk_qs * cs_h_th / 100, wk_qs * cs_l_th / 100);
	ctr->start();

	// join reader and writer threads
	reader->join();
	writer->join();

	// delete all threads
	delete reader;
	delete writer;
	delete ctr;
	delete transformer;

	// delete all queue
	delete input_queue;
	delete worker_queue;
	delete writer_queue;
}

/** 
 * Conduct a single experiment with assigned parameters
 * @param r_qs     reader queue size
 * @param wk_qs    worker queue size
 * @param wt_qs    writer queue size
 * @param cs_h_th  consumer controller high threshold size
 * @param cs_l_th  consumer controller low threshold size
 * @param cs_per   check period of consumer controller
 * @param in       input file name
 * @param out      output file name
 * @param log      log file name (pointer, since this is optional)
 * @param lines    numbers of lines in "in" file
 * @param to_print whether to print the experiment information
 */
void one_experiment(
	int r_qs, int wk_qs, int wt_qs, 
	int cs_h_th, int cs_l_th, int cs_per, 
	std::string & in, std::string & out, std::string * log, 
	int lines, bool to_print) 
{
	int pid = fork();
	
	if (!pid) {
		// is child process
		// experiment timer start
		auto start = std::chrono::high_resolution_clock::now();
		
		// start the experience
		one_run(
			r_qs, wk_qs, wt_qs, 
			cs_h_th, cs_l_th, cs_per, 
			in, out, lines);
		
		// experiment timer end
		auto end = std::chrono::high_resolution_clock::now();

		// calculate time consumption
		double result = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
		result *= 1e-9;
		
		if (to_print) {
			// print experiment message
			std::cout << "\nQueue size:\n    (reader, worker, writer) = (" 
				<< r_qs << ", " << wk_qs << ", " << wt_qs << ")" << std::endl;
			std::cout << "Consumer settings:\n    (high, low, period) = (" 
				<< cs_h_th << "%, " << cs_l_th << "%, ";
			if (cs_per > 1000000) 
				std::cout << (cs_per / 1000000.) << "s)" << std::endl;
			else
				std::cout << (cs_per / 1000.) << "ms)" << std::endl;
			// print time consumption
			std::cout << "Total time usage: " 
				<< result << std::setprecision(6) 
				<< "s" << std::endl << std::endl;
		}
		
		// write experiment message to log file
		if (log) {
			std::ofstream log_file(*log, std::ios::app);
			log_file << result << " / [";
			for (auto n: *rec_ptr) 
				log_file << n << ", ";
			log_file << "]" << std::endl;
			log_file.close();
		}
		
		exit(0); // end of child process
	}
	// wait for child process to end
	wait(nullptr);
}

int main(int argc, char** argv) {
	// TODO: implements main function
	// parse the parameters
	std::unordered_set<std::string> args; // arguments
	for (int i = 0; i < argc; ++i)
		args.insert(argv[i]);

	// identify the assigned functions
	bool to_exp = args.find("expe") != args.end();
	bool to_debug = args.find("msg") != args.end();

	// check if all the arguments are valid 
	bool all_args_valid = to_exp + to_debug + 4 == argc;
	
	// check legalness of arguments
	assert(all_args_valid);

	int n = atoi(argv[1]); // expected lines
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);

	// rename macros for better experiment convenience
	int R_QS  = READER_QUEUE_SIZE, // reader queue size
		WK_QS = WORKER_QUEUE_SIZE, // worker queue size
		WT_QS = WRITER_QUEUE_SIZE,  // writer queue size
		CS_H_TH = CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE, // consumer controller high threshold percentage 
		CS_L_TH = CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE, // consumer controller low threshold percentage
		CS_PER  = CONSUMER_CONTROLLER_CHECK_PERIOD; // consumer controller check period

	// basic run
	one_experiment(
		R_QS, WK_QS, WT_QS, 
		CS_H_TH, CS_L_TH, CS_PER, 
		input_file_name, output_file_name, 
		nullptr, n, false);

	// conduct all experiments
	if (to_exp) {
		// variables for experience
		const int expe_times = 8;
		std::string out_file("./tmp/tmp.out"); // dami output file

		// assign global variables for experiment
		rec_ptr = new std::vector<int>();
		print_scaling_msg = false;

		// experiences
		{
			std::cout << "----------- [start of period test] ----------" << std::endl;
			std::string log_name("./report/log/period.log");
			std::ofstream log_file(log_name);
			log_file << "Period / Time Usage / Consumer Size Change List" << std::endl;
			log_file.close();
			for (long long i = 1 << 10; i <= 1 << 24; i <<= 2) {
				for (int j = 0; j < expe_times; ++j) {
					log_file.open(log_name, std::ios::app);
					log_file << i << " / ";
					log_file.close();
					one_experiment(
						R_QS, WK_QS, WT_QS, 
						CS_H_TH, CS_L_TH, i, 
						input_file_name, out_file, 
						&log_name, n, to_debug);
				}
			}
			std::cout << "------------ [end of period test] -----------" << std::endl << std::endl;
		}

		{
			std::cout << "-------  [start of low threshold test] ------" << std::endl;
			std::string log_name("./report/log/low_threshold.log");
			std::ofstream log_file(log_name);
			log_file << "Low Threshold / Time Usage / Consumer Size Change List" << std::endl;
			log_file.close();
			for (int i = 0; i < CS_H_TH; i += 5) {
				for (int j = 0; j < expe_times; ++j) {
					log_file.open(log_name, std::ios::app);
					log_file << i << " / ";
					log_file.close();
					one_experiment(
						R_QS, WK_QS, WT_QS, 
						CS_H_TH, i, CS_PER, 
						input_file_name, out_file, 
						&log_name, n, to_debug);
				}
			}
			std::cout << "--------- [end of low threshold test] -------" << std::endl << std::endl;
		}
		
		{
			std::cout << "------  [start of high threshold test] ------" << std::endl;
			std::string log_name("./report/log/high_threshold.log");
			std::ofstream log_file(log_name);
			log_file << "High Threshold / Time Usage / Consumer Size Change List" << std::endl;
			log_file.close();
			for (int i = CS_L_TH + 5; i <= 100; i += 5) {
				for (int j = 0; j < expe_times; ++j) {
					log_file.open(log_name, std::ios::app);
					log_file << i << " / ";
					log_file.close();
					one_experiment(
						R_QS, WK_QS, WT_QS, 
						i, CS_L_TH, CS_PER, 
						input_file_name, out_file, 
						&log_name, n, to_debug);
				}
			}
			std::cout << "-------- [end of high threshold test] -------" << std::endl << std::endl;
		}
		
		{
			std::cout << "-------- [start of worker queue test] -------" << std::endl;
			std::string log_name("./report/log/work_size.log");
			std::ofstream log_file(log_name);
			log_file << "Worker Queue Size / Time Usage / Consumer Size Change List" << std::endl;
			log_file.close();
			for (int i = 20; i <= 400; i += 20) {
				for (int j = 0; j < expe_times; ++j) {
					log_file.open(log_name, std::ios::app);
					log_file << i << " / ";
					log_file.close();
					one_experiment(
						R_QS, i, WT_QS, 
						CS_H_TH, CS_L_TH, CS_PER, 
						input_file_name, out_file, 
						&log_name, n, to_debug);
				}
			}
			std::cout << "--------- [end of worker queue test] --------" << std::endl << std::endl;
		}
		
		{
			std::cout << "-------- [start of writer queue test] -------" << std::endl;
			std::string log_name("./report/log/write_size.log");
			std::ofstream log_file(log_name);
			log_file << "Writer Queue Size / Time Usage / Consumer Size Change List" << std::endl;
			log_file.close();
			for (int i = 250; i <= 7500; i += 250) {
				for (int j = 0; j < expe_times; ++j) {
					log_file.open(log_name, std::ios::app);
					log_file << i << " / ";
					log_file.close();
					one_experiment(
						R_QS, WK_QS, i, 
						CS_H_TH, CS_L_TH, CS_PER, 
						input_file_name, out_file, 
						&log_name, n, to_debug);
				}
			}
			std::cout << "--------- [end of writer queue test] --------" << std::endl << std::endl;
		}
		
		{
			std::cout << "-------- [start of reader queue test] -------" << std::endl;
			std::string log_name("./report/log/read_size.log");
			std::ofstream log_file(log_name);
			log_file << "Reader Queue Size / Time Usage / Consumer Size Change List" << std::endl;
			log_file.close();
			for (int i = 5; i <= 400; i += 5) {
				for (int j = 0; j < expe_times; ++j) {
					log_file.open(log_name, std::ios::app);
					log_file << i << " / ";
					log_file.close();
					one_experiment(
						i, WK_QS, WT_QS, 
						CS_H_TH, CS_L_TH, CS_PER, 
						input_file_name, out_file, 
						&log_name, n, to_debug);
				}
			}
			std::cout << "--------- [end of reader queue test] --------" << std::endl << std::endl;
		}

		delete rec_ptr;
	}

	return 0;
}
