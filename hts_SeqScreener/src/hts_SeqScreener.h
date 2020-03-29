/*The idea of this program is to screen out reads that "look like" a reference
 * this is accomplised by making a lookup table (hash table) that uses kmer binaries as their hash, and if enough Kmers
 * "hit" this table, it is considered close enough, and is discarded
 */
#ifndef SEQSCREENER_H
#define SEQSCREENER_H

#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include "utils.h"
#include "ioHandler.h"
#include "counters.h"
#include "main_template.h"

#include <map>
#include <unordered_map>
#include <algorithm>
#include <bitset>
#include <unordered_set>
#include <boost/dynamic_bitset.hpp>
#include <boost/functional/hash.hpp>
#include <tuple>

extern template class InputReader<SingleEndRead, SingleEndReadFastqImpl>;
extern template class InputReader<PairedEndRead, PairedEndReadFastqImpl>;
extern template class InputReader<PairedEndRead, InterReadImpl>;
extern template class InputReader<ReadBase, TabReadImpl>;

const std::string phixSeq_True = "GAGTTTTATCGCTTCCATGACGCAGAAGTTAACACTTTCGGATATTTCTGATGAGTCGAAAAATTATCTTGATAAAGCAGGAATTACTACTGCTTGTTTACGAATTAAATCGAAGTGGACTGCTGGCGGAAAATGAGAAAATTCGACCTATCCTTGCGCAGCTCGAGAAGCTCTTACTTTGCGACCTTTCGCCATCAACTAACGATTCTGTCAAAAACTGACGCGTTGGATGAGGAGAAGTGGCTTAATATGCTTGGCACGTTCGTCAAGGACTGGTTTAGATATGAGTCACATTTTGTTCATGGTAGAGATTCTCTTGTTGACATTTTAAAAGAGCGTGGATTACTATCTGAGTCCGATGCTGTTCAACCACTAATAGGTAAGAAATCATGAGTCAAGTTACTGAACAATCCGTACGTTTCCAGACCGCTTTGGCCTCTATTAAGCTCATTCAGGCTTCTGCCGTTTTGGATTTAACCGAAGATGATTTCGATTTTCTGACGAGTAACAAAGTTTGGATTGCTACTGACCGCTCTCGTGCTCGTCGCTGCGTTGAGGCTTGCGTTTATGGTACGCTGGACTTTGTGGGATACCCTCGCTTTCCTGCTCCTGTTGAGTTTATTGCTGCCGTCATTGCTTATTATGTTCATCCCGTCAACATTCAAACGGCCTGTCTCATCATGGAAGGCGCTGAATTTACGGAAAACATTATTAATGGCGTCGAGCGTCCGGTTAAAGCCGCTGAATTGTTCGCGTTTACCTTGCGTGTACGCGCAGGAAACACTGACGTTCTTACTGACGCAGAAGAAAACGTGCGTCAAAAATTACGTGCGGAAGGAGTGATGTAATGTCTAAAGGTAAAAAACGTTCTGGCGCTCGCCCTGGTCGTCCGCAGCCGTTGCGAGGTACTAAAGGCAAGCGTAAAGGCGCTCGTCTTTGGTATGTAGGTGGTCAACAATTTTAATTGCAGGGGCTTCGGCCCCTTACTTGAGGATAAATTATGTCTAATATTCAAACTGGCGCCGAGCGTATGCCGCATGACCTTTCCCATCTTGGCTTCCTTGCTGGTCAGATTGGTCGTCTTATTACCATTTCAACTACTCCGGTTATCGCTGGCGACTCCTTCGAGATGGACGCCGTTGGCGCTCTCCGTCTTTCTCCATTGCGTCGTGGCCTTGCTATTGACTCTACTGTAGACATTTTTACTTTTTATGTCCCTCATCGTCACGTTTATGGTGAACAGTGGATTAAGTTCATGAAGGATGGTGTTAATGCCACTCCTCTCCCGACTGTTAACACTACTGGTTATATTGACCATGCCGCTTTTCTTGGCACGATTAACCCTGATACCAATAAAATCCCTAAGCATTTGTTTCAGGGTTATTTGAATATCTATAACAACTATTTTAAAGCGCCGTGGATGCCTGACCGTACCGAGGCTAACCCTAATGAGCTTAATCAAGATGATGCTCGTTATGGTTTCCGTTGCTGCCATCTCAAAAACATTTGGACTGCTCCGCTTCCTCCTGAGACTGAGCTTTCTCGCCAAATGACGACTTCTACCACATCTATTGACATTATGGGTCTGCAAGCTGCTTATGCTAATTTGCATACTGACCAAGAACGTGATTACTTCATGCAGCGTTACCATGATGTTATTTCTTCATTTGGAGGTAAAACCTCTTATGACGCTGACAACCGTCCTTTACTTGTCATGCGCTCTAATCTCTGGGCATCTGGCTATGATGTTGATGGAACTGACCAAACGTCGTTAGGCCAGTTTTCTGGTCGTGTTCAACAGACCTATAAACATTCTGTGCCGCGTTTCTTTGTTCCTGAGCATGGCACTATGTTTACTCTTGCGCTTGTTCGTTTTCCGCCTACTGCGACTAAAGAGATTCAGTACCTTAACGCTAAAGGTGCTTTGACTTATACCGATATTGCTGGCGACCCTGTTTTGTATGGCAACTTGCCGCCGCGTGAAATTTCTATGAAGGATGTTTTCCGTTCTGGTGATTCGTCTAAGAAGTTTAAGATTGCTGAGGGTCAGTGGTATCGTTATGCGCCTTCGTATGTTTCTCCTGCTTATCACCTTCTTGAAGGCTTCCCATTCATTCAGGAACCGCCTTCTGGTGATTTGCAAGAACGCGTACTTATTCGCCACCATGATTATGACCAGTGTTTCCAGTCCGTTCAGTTGTTGCAGTGGAATAGTCAGGTTAAATTTAATGTGACCGTTTATCGCAATCTGCCGACCACTCGCGATTCAATCATGACTTCGTGATAAAAGATTGAGTGTGAGGTTATAACGCCGAAGCGGTAAAAATTTTAATTTTTGCCGCTGAGGGGTTGACCAAGCGAAGCGCGGTAGGTTTTCTGCTTAGGAGTTTAATCATGTTTCAGACTTTTATTTCTCGCCATAATTCAAACTTTTTTTCTGATAAGCTGGTTCTCACTTCTGTTACTCCAGCTTCTTCGGCACCTGTTTTACAGACACCTAAAGCTACATCGTCAACGTTATATTTTGATAGTTTGACGGTTAATGCTGGTAATGGTGGTTTTCTTCATTGCATTCAGATGGATACATCTGTCAACGCCGCTAATCAGGTTGTTTCTGTTGGTGCTGATATTGCTTTTGATGCCGACCCTAAATTTTTTGCCTGTTTGGTTCGCTTTGAGTCTTCTTCGGTTCCGACTACCCTCCCGACTGCCTATGATGTTTATCCTTTGAATGGTCGCCATGATGGTGGTTATTATACCGTCAAGGACTGTGTGACTATTGACGTCCTTCCCCGTACGCCGGGCAATAACGTTTATGTTGGTTTCATGGTTTGGTCTAACTTTACCGCTACTAAATGCCGCGGATTGGTTTCGCTGAATCAGGTTATTAAAGAGATTATTTGTCTCCAGCCACTTAAGTGAGGTGATTTATGTTTGGTGCTATTGCTGGCGGTATTGCTTCTGCTCTTGCTGGTGGCGCCATGTCTAAATTGTTTGGAGGCGGTCAAAAAGCCGCCTCCGGTGGCATTCAAGGTGATGTGCTTGCTACCGATAACAATACTGTAGGCATGGGTGATGCTGGTATTAAATCTGCCATTCAAGGCTCTAATGTTCCTAACCCTGATGAGGCCGCCCCTAGTTTTGTTTCTGGTGCTATGGCTAAAGCTGGTAAAGGACTTCTTGAAGGTACGTTGCAGGCTGGCACTTCTGCCGTTTCTGATAAGTTGCTTGATTTGGTTGGACTTGGTGGCAAGTCTGCCGCTGATAAAGGAAAGGATACTCGTGATTATCTTGCTGCTGCATTTCCTGAGCTTAATGCTTGGGAGCGTGCTGGTGCTGATGCTTCCTCTGCTGGTATGGTTGACGCCGGATTTGAGAATCAAAAAGAGCTTACTAAAATGCAACTGGACAATCAGAAAGAGATTGCCGAGATGCAAAATGAGACTCAAAAAGAGATTGCTGGCATTCAGTCGGCGACTTCACGCCAGAATACGAAAGACCAGGTATATGCACAAAATGAGATGCTTGCTTATCAACAGAAGGAGTCTACTGCTCGCGTTGCGTCTATTATGGAAAACACCAATCTTTCCAAGCAACAGCAGGTTTCCGAGATTATGCGCCAAATGCTTACTCAAGCTCAAACGGCTGGTCAGTATTTTACCAATGACCAAATCAAAGAAATGACTCGCAAGGTTAGTGCTGAGGTTGACTTAGTTCATCAGCAAACGCAGAATCAGCGGTATGGCTCTTCTCATATTGGCGCTACTGCAAAGGATATTTCTAATGTCGTCACTGATGCTGCTTCTGGTGTGGTTGATATTTTTCATGGTATTGATAAAGCTGTTGCCGATACTTGGAACAATTTCTGGAAAGACGGTAAAGCTGATGGTATTGGCTCTAATTTGTCTAGGAAATAACCGTCAGGATTGACACCCTCCCAATTGTATGTTTTCATGCCTCCAAATCTTGGAGGCTTTTTTATGGTTCGTTCTTATTACCCTTCTGAATGTCACGCTGATTATTTTGACTTTGAGCGTATCGAGGCTCTTAAACCTGCTATTGAGGCTTGTGGCATTTCTACTCTTTCTCAATCCCCAATGCTTGGCTTCCATAAGCAGATGGATAACCGCATCAAGCTCTTGGAAGAGATTCTGTCTTTTCGTATGCAGGGCGTTGAGTTCGATAATGGTGATATGTATGTTGACGGCCATAAGGCTGCTTCTGACGTTCGTGATGAGTTTGTATCTGTTACTGAGAAGTTAATGGATGAATTGGCACAATGCTACAATGTGCTCCCCCAACTTGATATTAATAACACTATAGACCACCGCCCCGAAGGGGACGAAAAATGGTTTTTAGAGAACGAGAAGACGGTTACGCAGTTTTGCCGCAAGCTGGCTGCTGAACGCCCTCTTAAGGATATTCGCGATGAGTATAATTACCCCAAAAAGAAAGGTATTAAGGATGAGTGTTCAAGATTGCTGGAGGCCTCCACTATGAAATCGCGTAGAGGCTTTGCTATTCAGCGTTTGATGAATGCAATGCGACAGGCTCATGCTGATGGTTGGTTTATCGTTTTTGACACTCTCACGTTGGCTGACGACCGATTAGAGGCGTTTTATGATAATCCCAATGCTTTGCGTGACTATTTTCGTGATATTGGTCGTATGGTTCTTGCTGCCGAGGGTCGCAAGGCTAATGATTCACACGCCGACTGCTATCAGTATTTTTGTGTGCCTGAGTATGGTACAGCTAATGGCCGTCTTCATTTCCATGCGGTGCACTTTATGCGGACACTTCCTACAGGTAGCGTTGACCCTAATTTTGGTCGTCGGGTACGCAATCGCCGCCAGTTAAATAGCTTGCAAAATACGTGGCCTTATGGTTACAGTATGCCCATCGCAGTTCGCTACACGCAGGACGCTTTTTCACGTTCTGGTTGGTTGTGGCCTGTTGATGCTAAAGGTGAGCCGCTTAAAGCTACCAGTTATATGGCTGTTGGTTTCTATGTGGCTAAATACGTTAACAAAAAGTCAGATATGGACCTTGCTGCTAAAGGTCTAGGAGCTAAAGAATGGAACAACTCACTAAAAACCAAGCTGTCGCTACTTCCCAAGAAGCTGTTCAGAATCAGAATGAGCCGCAACTTCGGGATGAAAATGCTCACAATGACAAATCTGTCCACGGAGTGCTTAATCCAACTTACCAAGCTGGGTTACGACGCGACGCCGTTCAACCAGATATTGAAGCAGAACGCAAAAAGAGAGATGAGATTGAGGCTGGGAAAAGTTACTGTAGCCGACGTTTTGGCGGCGCAACCTGTGACGACAAATCTGCTCAAATTTATGCGCGCTTCGATAAAAATGATTGGCGTATCCAACCTGCA";

class SeqScreenerCounters : public Counters {

public:
    std::string screen_file;
    uint64_t screen_bp = 0;
    uint64_t lookup_kmers = 0;
    std::vector<Label> screened_info;

    uint64_t Inverse = 0;
    uint64_t Record = 0;

    uint64_t SE_hits = 0;

    uint64_t PE_hits = 0;

    SeqScreenerCounters(const std::string &program_name, const po::variables_map &vm) : Counters::Counters(program_name, vm) {
        generic.push_back(std::forward_as_tuple("inverse", Inverse));
        generic.push_back(std::forward_as_tuple("record", Record));

        se.push_back(std::forward_as_tuple("SE_hits", SE_hits));

        pe.push_back(std::forward_as_tuple("PE_hits", PE_hits));
        // Need to and in screen_file name, however its a string and the vector can't have mixed types.
        //screened_info.push_back(std::forward_as_tuple("screenFile", screen_file));
        screened_info.push_back(std::forward_as_tuple("screenBP", screen_bp));
        screened_info.push_back(std::forward_as_tuple("lookupKmers", lookup_kmers));

    }

    void set_screeninfo(const std::string &screenFile, uint64_t sbp, uint64_t lkmers) {
        screen_file = screenFile;
        screen_bp = sbp;
        lookup_kmers = lkmers;
    }
    void set_inverse() {
        Inverse = 1;
    }
    void set_record() {
        Record = 1;
    }
    void inc_SE_hits() {
        ++SE_hits;
    }
    void inc_PE_hits() {
        ++PE_hits;
    }
    virtual void write_out() {

        initialize_json();

        write_labels(generic);
        write_sublabels("Screen_info", screened_info);
        write_sublabels("Single_end", se);
        write_sublabels("Paired_end", pe);

        finalize_json();
    }
};

class dbhash {
public:
    std::size_t operator() ( const boost::dynamic_bitset<>& bs) const {
        return boost::hash_value(bs.m_bits);
    }
};

typedef std::unordered_set < boost::dynamic_bitset<>, dbhash> kmerSet;

class SeqScreener: public MainTemplate<SeqScreenerCounters, SeqScreener> {
public:

    SeqScreener() {
        program_name = "hts_SeqScreener";
        app_description =
            "hts_SeqScreener identifies and removes any reads which appear to have originated\n";
        app_description += "  from a contaminant DNA source. Because bacteriophage Phi-X is common spiked\n";
        app_description += "  into Illumina runs for QC purposes, sequences originating from Phi-X are removed\n";
        app_description += "  by default. If other contaminants are suspected their sequence can be supplied\n";
        app_description += "  as a fasta file <seq>, however the algorithm has been tuned for short contaminant\n";
        app_description += "  sequences, and may not work well with sequences significantly longer than Phi-X (5Kb).\n";
    }

    void add_extra_options(po::options_description &desc) {
        desc.add_options()
            ("seq,s", po::value<std::string>(), "Please supply a fasta file - default - Phix Sequence - default https://www.ncbi.nlm.nih.gov/nuccore/9626372")
            ("check-read-2,C", po::bool_switch()->default_value(false), "Check R2 as well as R1 (pe)")
            ("kmer,k", po::value<size_t>()->default_value(12)->notifier(boost::bind(&check_range<size_t>, "kmer", _1, 5, 256)), "Kmer size of the lookup table (min 5, max 256)")
            ("percentage-hits,x", po::value<double>()->default_value(.25)->notifier(boost::bind(&check_range<double>, "percentage-hits", _1, 0.0, 1.0)), "Proportion of kmer percentage-hits to sequence need to happen to discard (min 0.0, max 1.0)")
            ("inverse,n", po::bool_switch()->default_value(false), "Output reads that are ABOVE the kmer hit threshold")
            ("record,r", po::bool_switch()->default_value(false), "Only record the reads that pass the kmer hit threshold, output all reads");
    }

    void setBitsBools(boost::dynamic_bitset<> &bs, size_t loc, bool set1, bool set2) {
        bs[loc] = set1;
        bs[loc + 1] = set2;
    }

    std::pair <bool, bool> setBitsChar(char c) {

        switch (std::toupper(c)) {
        case 'A':
            return std::pair<bool, bool> (0, 0);
        case 'T':
            return std::pair<bool, bool> (1, 1);
        case 'C':
            return std::pair<bool, bool> (0, 1);
        case 'G':
            return std::pair<bool, bool> (1, 0);
        default:
            throw std::runtime_error(std::string("Unknown base pair in sequence ") + c);
        }
    }


/*The intent of this is to check  reads against the lookup table and return the number of hits
 * The function arguments are long because I did not want to recalculate those values each time
 * Especially reallocating the bitsets each time was costly.
 * Bitsets are safe, all positions will be overwritten without need to reset
 *
 * Two pairs of critical bitsets are at a work forwardLookup and forwardRest AND reverseLookup and reverseRest
 * the *Lookup varaibles are used as a 'prefix search'.
 *      We go to the prefix search location in the hashmap
 *          If diff (meaning the prefix DOES NOT contains all the kmer in the search) AND that location is not null
 *              check the vector at that location for *Rest (the rest of the data that isn't contained in Lookup)
 *              If the vector contains *Rest ++hit
 *
 *          else if !diff (lookup is the same size as KMER) AND that location is not null
 *              ++hit
 *
 * return number of hits seen */

    unsigned int check_read( kmerSet &lookup, const Read &rb, const size_t bitKmer, const size_t lookup_loc, const size_t lookup_loc_rc, boost::dynamic_bitset<> &forwardLookup, boost::dynamic_bitset<> &reverseLookup) {

        std::string seq = rb.get_seq();

        unsigned int hits = 0;
        unsigned int current_added = 0;

        /*The flow of base paires will be
         * Forward add to 0 and 1 location then << 2
         * Once the ForwardLookup is full and start filling ForwardRest
         * ForwardRest will take the highest location of ForwardLookup and put it
         * in the lowest bit of ForwardRest then shift << 2*/

        /*The flow for RC is a bit different
         *The bits go into RestReverse first (if there is a diff is greater than 0)
         *They start of the highest bits in Reverse and flow downward >>= 2
         *Once RestReverse if "full", the lowest bits will be put into the Highest point of LookupRest and shifted down >>=2
         *if diff is 0 - then LookupRest is the only thing that is filled
         *These RC bits are filled iwth the RC( *bp )*/

        /*These the lookups are compared, the larger Lookup is taken and searched for,
         * if there is a "soft hit", initiate a search of the vector with the cooresponding Rest*/
        std::pair <bool, bool> bits;
        for (std::string::iterator bp = seq.begin(); bp < seq.end(); ++bp) { //goes through each bp of the read

            if (std::toupper(*bp) == 'N') { // N resets everythign
                current_added = 0;
            } else {
                reverseLookup >>= 2;
                forwardLookup <<=2; //No special condition needed for Forward Looup
                bits = setBitsChar(*bp);
                forwardLookup[lookup_loc] = bits.first;
                forwardLookup[lookup_loc+1] = bits.second;
                reverseLookup[lookup_loc_rc ] = !bits.first;
                reverseLookup[lookup_loc_rc + 1] = !bits.second;

                current_added += 2;

                if (current_added >= bitKmer) { //initiate hit search
                    boost::dynamic_bitset<> &bs = forwardLookup > reverseLookup ? forwardLookup : reverseLookup;
                    if (lookup.find(bs) != lookup.end()) {
                        ++hits;
                    }
                }

            }
        }

        return hits;
    }

    template <class T, class Impl>
    void do_app(InputReader<T, Impl> &reader, std::shared_ptr<OutputWriter> pe, std::shared_ptr<OutputWriter> se, SeqScreenerCounters& counter, const po::variables_map &vm) {

        double hits = vm["percentage-hits"].as<double>();
        bool checkR2 = vm["check-read-2"].as<bool>();
        size_t kmerSize = vm["kmer"].as<size_t>();
        bool inverse = vm["inverse"].as<bool>();
        bool record = vm["record"].as<bool>();

    //sets read information
    //Phix isn't set to default since it makes help a PITA to read
    //sets kmer lookup arrays
    kmerSet lookup;
    uint64_t screen_len;
    std::string lookup_file;
    if (vm.count("seq")) {
        lookup_file = vm["seq"].as<std::string>();
        bi::stream <bi::file_descriptor_source> fa{check_open_r(lookup_file), bi::close_handle};
        InputReader<SingleEndRead, FastaReadImpl> faReader(fa);
        screen_len = setLookup_fasta(lookup, faReader, vm["kmer"].as<size_t>());
    } else {
        Read readSeq;
        lookup_file = "PhiX";
        readSeq = Read(phixSeq_True, "", "");
        screen_len = readSeq.getLength();
        setLookup_read(lookup, readSeq, vm["kmer"].as<size_t>());
    }
    if (lookup.size() == 0){
        throw std::runtime_error("Exception lookup table contains no kmers");
    }

    counter.set_screeninfo(lookup_file, screen_len, lookup.size());

    /*These are set here so each read doesn't have to recalcuate these stats*/

    size_t bitKmer = kmerSize * 2;

    size_t lookup_loc = 0; // change bit 0 and 1 the << 2
    size_t lookup_loc_rc = bitKmer - 2; // change bit 31 and 30 then >> 2

    /*Particullary time consuming in each read*/
    boost::dynamic_bitset <> forwardLookup(bitKmer);
    boost::dynamic_bitset <> reverseLookup(bitKmer);

    if (inverse && !record) counter.set_inverse();
    if (record) counter.set_record();

    while(reader.has_next()) {
        auto i = reader.next();
        PairedEndRead* per = dynamic_cast<PairedEndRead*>(i.get());
        if (per) {
            counter.input(*per);
            double val = check_read(lookup, per->get_read_one(), bitKmer, lookup_loc, lookup_loc_rc, forwardLookup, reverseLookup );
            val = val / ( per->get_read_one().getLength() - kmerSize);

            if (checkR2) {
                double val2 = check_read(lookup, per->get_read_two(), bitKmer, lookup_loc, lookup_loc_rc, forwardLookup, reverseLookup );

                val2 = val2 / (per->get_read_one().getLength() - kmerSize);
                val = std::max(val, val2);
            }

            if (val > hits) {
                counter.inc_PE_hits();
            }
            if (val <= hits && !inverse && !record) {
                counter.output(*per);
                writer_helper(per, pe, se, false);
            } else if (val > hits && inverse && !record) {
                counter.output(*per);
                writer_helper(per, pe, se, false);
            } else if (record) {
                counter.output(*per);
                writer_helper(per, pe, se, false);
            }

        } else {
            SingleEndRead* ser = dynamic_cast<SingleEndRead*>(i.get());

            if (ser) {
                counter.input(*ser);
                double val = check_read(lookup, ser->get_read(), bitKmer, lookup_loc, lookup_loc_rc, forwardLookup, reverseLookup );
                val = val / ( ser->get_read().getLength() - kmerSize);

                if (val > hits) {
                    counter.inc_SE_hits();
                }
                if (val <= hits && !inverse && !record) {
                    counter.output(*ser);
                    writer_helper(ser, pe, se, false);
                } else if (val > hits && inverse && !record) {
                    counter.output(*ser);
                    writer_helper(ser, pe, se, false);
                } else if (record) {
                    counter.output(*ser);
                    writer_helper(ser, pe, se, false);
                }
            } else {
                throw std::runtime_error("Unknown read type");
            }
        }
    }
}

/*The flow of base pairs will be
 * Forward add to 0 and 1 location then << 2
 * Once the ForwardLookup is full and start filling ForwardRest
 * ForwardRest will take the highest location of ForwardLookup and put it
 * in the lowest bit of ForwardRest then shift << 2*/

/*The flow for RC is a bit different
 *The bits go into RestReverse first (if there is a diff is greater than 0)
 *They start of the highest bits in Reverse and flow downward >>= 2
 *Once RestReverse if "full", the lowest bits will be put into the Highest point of LookupRest and shifted down >>=2
 *if diff is 0 - then LookupRest is the only thing that is filled
 *These RC bits are filled iwth the RC( *bp )*/

/*These the lookups are compared, the larger Lookup is taken and searched for,
 * if there is a "soft hit", initiate a search of the vector with the cooresponding Rest*/

    void setLookup_read( kmerSet &lookup, Read &rb, size_t kmerSize) {
        /*This function is only called once, these values calculation are minimal cost so they are not in the function args*/
        /*Total size of kmer bits*/
        size_t bitKmer = kmerSize * 2;

        /*lookup locations*/
        size_t lookup_loc = 0;
        size_t lookup_loc_rc = bitKmer - 2;

        size_t current_added = 0;
        boost::dynamic_bitset <> forwardLookup(bitKmer);
        boost::dynamic_bitset <> reverseLookup(bitKmer);

        std::string seq = rb.get_seq();
        std::pair<bool, bool> bits;
        for (std::string::iterator bp = seq.begin(); bp != seq.end(); ++bp) {
            if (std::toupper(*bp) == 'A' || std::toupper(*bp) == 'C' || std::toupper(*bp) == 'G' || std::toupper(*bp) == 'T' ) {
                reverseLookup >>= 2;
                forwardLookup <<=2;

                bits = setBitsChar(*bp);
                forwardLookup[lookup_loc] = bits.first;
                forwardLookup[lookup_loc + 1] = bits.second;
                reverseLookup[lookup_loc_rc] = !bits.first;
                reverseLookup[lookup_loc_rc + 1] = !bits.second;
                current_added += 2;
                if (current_added >= bitKmer) {
                    boost::dynamic_bitset<> &bs = forwardLookup > reverseLookup ? forwardLookup : reverseLookup;
                    lookup.insert(bs);
                }
            } else { // all other characters (N, ambiguities) produce no kmers
                current_added = 0;
            }
        }

    }

uint64_t setLookup_fasta( kmerSet &lookup, InputReader<SingleEndRead, FastaReadImpl> &faReader, size_t kmerSize ) {
    uint64_t slen = 0;
    while(faReader.has_next()) {
        auto r = faReader.next();
        Read readSeq = Read(r->get_read().get_seq(), "", "");
        slen += readSeq.getLength();
        setLookup_read(lookup, readSeq, kmerSize);
    }
    return slen;
}
};
#endif
