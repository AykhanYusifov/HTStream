#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <vector>
#include <fstream>
#include "ioHandler.h"
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <unordered_map>
#include <boost/functional/hash.hpp>

#include "adapter-trimmer.h"

#define AST_COUNT 4096 //number of reads per one *

namespace
{
    const size_t SUCCESS = 0;
    const size_t ERROR_IN_COMMAND_LINE = 1;
    const size_t ERROR_UNHANDLED_EXCEPTION = 2;

} // namespace

namespace bi = boost::iostreams;

int main(int argc, char** argv)
{
    const std::string program_name = "adapter_trimmer";

    AdapterTrimmerCounters counters;

    try
    {
        /** Define and parse the program options
         */
        namespace po = boost::program_options;
        po::options_description desc("Options");

        setDefaultParams(desc, program_name);
        setDefaultParamsCutting(desc);

        desc.add_options()
            ("kmer,k", po::value<size_t>()->default_value(8), "Kmer size of the lookup table for the longer read")
            ("kmer-offset,r", po::value<size_t>()->default_value(1), "Offset of kmers. Offset of 1, would be perfect overlapping kmers. An offset of kmer would be non-overlapping kmers that are right next to each other. Must be greater than 0.")
            ("max-mismatch-errorDensity,x", po::value<double>()->default_value(.25), "Max percent of mismatches allowed in overlapped section")
            ("check-lengths,c", po::value<size_t>()->default_value(20), "Check lengths on the ends")
            ("min-overlap,o", po::value<size_t>()->default_value(8), "Min overlap required to merge two reads");

                   po::variables_map vm;
        try
        {
            po::store(po::parse_command_line(argc, argv, desc),
                      vm); // can throw
            
            version_or_help(program_name, desc, vm);

            po::notify(vm); // throws on error, so do after help in case
         
            if (vm["max-mismatch-errorDensity"].as<double>() < 0.0 ||  vm["max-mismatch-errorDensity"].as<double>() > 1.0) {
                throw std::runtime_error("Woah, there human. It seems you have entered a wacky, zany number that isn't between 0.0 and 1.0 for max mismatch errorDensity.\nThat is why there is this error message. (>^_^)>\n\n");
            }

            if (vm["kmer-offset"].as<size_t>() == 0) {
                throw std::runtime_error("Human - you cannot have a kmer offset of zero! Madness would ensue. Please, try again, but this time make sure the kmer-offset,r flag is set to above 0.\n\n");
            }
        
            std::string statsFile(vm["stats-file"].as<std::string>());
            std::string prefix(vm["prefix"].as<std::string>());
   
            std::shared_ptr<OutputWriter> pe = nullptr;
            std::shared_ptr<OutputWriter> se = nullptr;
            
            outputWriters(pe, se, vm["fastq-output"].as<bool>(), vm["tab-output"].as<bool>(), vm["interleaved-output"].as<bool>(), vm["unmapped-output"].as<bool>(), vm["force"].as<bool>(), vm["gzip-output"].as<bool>(), vm["to-stdout"].as<bool>(), prefix );

            if(vm.count("read1-input")) {
                if (!vm.count("read2-input")) {
                    throw std::runtime_error("must specify both read1 and read2 input files.");
                } else if (vm.count("read2-input") != vm.count("read1-input")) {
                    throw std::runtime_error("must have same number of input files for read1 and read2");
                }
                auto read1_files = vm["read1-input"].as<std::vector<std::string> >();
                auto read2_files = vm["read2-input"].as<std::vector<std::string> >();

                for(size_t i = 0; i < read1_files.size(); ++i) {
                    bi::stream<bi::file_descriptor_source> is1{check_open_r(read1_files[i]), bi::close_handle};
                    bi::stream<bi::file_descriptor_source> is2{check_open_r(read2_files[i]), bi::close_handle};
                    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(is1, is2);
                    helper_overlapper(ifp, pe, se, counters, vm["max-mismatch-errorDensity"].as<double>(),  vm["min-overlap"].as<size_t>(), vm["stranded"].as<bool>(), vm["min-length"].as<size_t>(), vm["check-lengths"].as<size_t>(), vm["kmer"].as<size_t>(), vm["kmer-offset"].as<size_t>(), vm["no-orphans"].as<bool>() ); }
            }

            if(vm.count("singleend-input")) {
                auto read_files = vm["singleend-input"].as<std::vector<std::string> >();
                for (auto file : read_files) {
                    bi::stream<bi::file_descriptor_source> sef{ check_open_r(file), bi::close_handle};
                    InputReader<SingleEndRead, SingleEndReadFastqImpl> ifs(sef);
                    //JUST WRITE se read out - no way to overlap
                    helper_overlapper(ifs, pe, se, counters, vm["max-mismatch-errorDensity"].as<double>(),  vm["min-overlap"].as<size_t>(), vm["stranded"].as<bool>(), vm["min-length"].as<size_t>(), vm["check-lengths"].as<size_t>(), vm["kmer"].as<size_t>(), vm["kmer-offset"].as<size_t>(), vm["no-orphans"].as<bool>() );
                }
            }
            
            if(vm.count("tab-input")) {
                auto read_files = vm["tab-input"].as<std::vector<std::string> > ();
                for (auto file : read_files) {
                    bi::stream<bi::file_descriptor_source> tabin{ check_open_r(file), bi::close_handle};
                    InputReader<ReadBase, TabReadImpl> ift(tabin);
                    helper_overlapper(ift, pe, se, counters, vm["max-mismatch-errorDensity"].as<double>(),  vm["min-overlap"].as<size_t>(), vm["stranded"].as<bool>(), vm["min-length"].as<size_t>(), vm["check-lengths"].as<size_t>(), vm["kmer"].as<size_t>(), vm["kmer-offset"].as<size_t>(), vm["no-orphans"].as<bool>() );
                }
            }
            
            if (vm.count("interleaved-input")) {
                auto read_files = vm["interleaved-input"].as<std::vector<std::string > >();
                for (auto file : read_files) {
                    bi::stream<bi::file_descriptor_source> inter{ check_open_r(file), bi::close_handle};
                    InputReader<PairedEndRead, InterReadImpl> ifp(inter);
                    helper_overlapper(ifp, pe, se, counters, vm["max-mismatch-errorDensity"].as<double>(),  vm["min-overlap"].as<size_t>(), vm["stranded"].as<bool>(), vm["min-length"].as<size_t>(), vm["check-lengths"].as<size_t>(), vm["kmer"].as<size_t>(), vm["kmer-offset"].as<size_t>(), vm["no-orphans"].as<bool>() );
                }
            }
           
            if (vm.count("std-input")) {
                bi::stream<bi::file_descriptor_source> tabin {fileno(stdin), bi::close_handle};
                InputReader<ReadBase, TabReadImpl> ift(tabin);
                helper_overlapper(ift, pe, se, counters, vm["max-mismatch-errorDensity"].as<double>(),  vm["min-overlap"].as<size_t>(), vm["stranded"].as<bool>(), vm["min-length"].as<size_t>(), vm["check-lengths"].as<size_t>(), vm["kmer"].as<size_t>(), vm["kmer-offset"].as<size_t>(), vm["no-orphans"].as<bool>() );
            }  
            counters.write_out(statsFile, vm["append-stats-file"].as<bool>(), program_name, vm["notes"].as<std::string>());

        }
        catch(po::error& e)
        {
            std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
            std::cerr << desc << std::endl;
            return ERROR_IN_COMMAND_LINE;
        }

    }
    catch(std::exception& e)
    {
        std::cerr << "\n\tUnhandled Exception: "
                  << e.what() << std::endl;
        return ERROR_UNHANDLED_EXCEPTION;

    }

    return SUCCESS;

}
