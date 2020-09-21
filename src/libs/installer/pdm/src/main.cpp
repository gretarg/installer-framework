#include "../include/pdm.h"

#include <iostream>
#include <algorithm>
#include <fstream>

using namespace PDM;

void Output(const SubItem& item, std::ostream& stream, int indentation = 0);

void Output(const std::vector<DataField>& items, std::ostream& stream, int indentation)
{
	if (!items.size()) return;

	auto maxlen = std::max_element(cbegin(items), cend(items), [](const DataField& item1, const DataField& item2)
	{
		return item1.name.length() < item2.name.length();
	})->name.length();

	std::for_each(cbegin(items), cend(items), [&stream, indentation, maxlen](const DataField& item)
	{
		for (auto i = indentation; i--;) stream << '\t';
		auto& val = item.value.empty() ? "{EMPTY}" : item.value;
		stream << item.name;
		for (auto i = item.name.length(); i < maxlen; i++) stream << ' ';
		stream << ": " << val << '\n';
	});

	stream << '\n';
}

void Output(const std::vector<SubItem>& items, std::ostream& stream, int indentation)
{
	std::for_each(cbegin(items), cend(items), [&stream, indentation](const SubItem& item)
	{
		Output(item, stream, indentation);
	});
}

void Output(const SubItem& item, std::ostream& stream, int indentation)
{
	if (!item.items.size() && !item.subitems.size()) return;

	for (auto i = indentation; i--;) stream << '\t';
	stream << "{" << item.name << "}\n";

	Output(item.items, stream, indentation + 1);
	Output(item.subitems, stream, indentation + 1);
}

void Output(const PDMData& data, std::ostream& stream)
{
	Output(data.data, stream);
	stream.flush();
}

std::string TimestampToString(const TimeStamp& timestamp)
{
	const int MAX_SIZE = 20;
	char time[MAX_SIZE];
	strftime(time, MAX_SIZE, "%Y_%m_%d_%H_%M_%S", &timestamp);
	return time;
}

void OutputExecutionTimings(std::string filename)
{
	std::fstream outfile;
	outfile.open(filename, std::ios::out);

	if (outfile)
	{
		const unsigned threshold = 100;
		const unsigned runs = 100;

		outfile << "Runs: " << runs << " Threshold: " << threshold << "\n";
		
		uint64_t average = 0;
		unsigned crossings = 0;

		for (unsigned i = 0; i < runs; i++)
		{
			uint64_t cycles = GetTimingCycles();
			average += cycles;

			outfile << "Cycles: ";
			if (cycles > threshold)
			{
				outfile << "*";
				crossings++;
			}
			outfile << cycles << "\n";
		}

		average /= runs;

		outfile << "Average: " << average << " Crossings: " << crossings << std::endl;
		outfile.close();
	}
}

auto Execute()
{
	try
	{
		const auto& data = RetrievePDMData("pdmCLI", "1.0");

		std::fstream outfile;
		std::string filename = "PDM_Output_" + GetMachineName() + "_" + TimestampToString(data.timestamp);

		outfile.open(filename + ".txt", std::ios::out);

		if (!outfile)
		{
			std::cout << "Error: Could not open output file!\n\n"; // But dump to console anyway
		}
		else
		{
			Output(data, outfile);
			outfile.close();
		}

		Output(data, std::cout);

		OutputExecutionTimings(filename + "_VM_Execution_Timings.txt");
	}
	catch (std::exception & e)
	{
		std::cout << "Unexpected exception: " << e.what() << std::endl;

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void Wait()
{
	std::cout << "Press Enter to exit...";
	std::cin.get();
}

int main()
{
	auto code = Execute();
	Wait();
	return code;
}
