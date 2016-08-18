#include <gtest/gtest.h>
#include "ioHandler.h"
#include <sstream>
#include <iostream>

class ReadsTest : public ::testing::Test {
public:
    const std::string readData = "@M03610:15:000000000-ALE1U:1:1101:17702:1965 1:N:0:1\nCCTTTTGTTTGTATTGTTTCTCTTAAGCATATTTCTTAATTACTTAACTAACTACCTCAAATGAAATTTGTAAAAAATCCATCAATTTTAACCGCTTGTTTGCTTGCCAGTGTCCTCGGAGGCATTGGGACACTCGCCTACTCCCCTTTTG\n+\nAAAAAF5CFFFF6FGFGGGGGGHHGHHFBHFFHHHHHBEGHHHHHEGGHHHHHFGFGGFEFGHBFBHHHHHEEGFFEGHHFHHEBGHHHEGHHGEGGEHHHGHHHEHHFFG3GHHHHGCDEGGGHHDGEHHGHHHFEEGGFHHHHGHHHH1\n@M03610:15:000000000-ALE1U:1:1101:13392:1966 1:N:0:1\nGTCGCCGGATCTAATGCACTTGTCGCTTCATCGCATAATAAAACGTGCGGATCACTTGCCAACGCACGGGCAATCGCTACACGTTGTTTCTGACCACCGGATAAATTGCTCGGATAAACGTCTTTACGTTCAGTTAAGCCGACCAATTCAA\n+\n>AA3>A>22ADFC5FBGGGGGCGHGHGHGGHHHGGGGFHFFHFGGHHHGGGGEHHHHFHHGHGGGGGGGCGGFHHGGHGGHHGHHHGHHHHHHHHGHGGGGGGFHHHHHHHGGGGGHHHGHHHGHHHGHHHHHHHGGEGFC?DGGHHHHG0\n@M03610:15:000000000-ALE1U:1:1101:19181:1996 1:N:0:1\nATTTGTGGCAATGGCGAGTTTGGGGCGATACCAATATCCACACAATAGCCAGTAAACTTAACGTTAGCGGGCAAGGAAATTGGCACCAAAATTTGGTACAACTCAACAAATTAAGTGGTGAATTAGGCAAAATTCACCAGAAAGGCGTGTA\n+\n>AAA3DCAAFFFB4EAEFGGGE2AFGGGGFGHBGHFHHHFHHGGGHHHHHHHHHHHHHHHHHGHHEGHCGGGGGGGHHHHHFHHGHGHHHHHHHHGHHHHGHHHGHHEHGHHHHHGHGHGHHHHHHHHHHGHHHHHHHHH0GHGHHGGGGF\n@M03610:15:000000000-ALE1U:1:1101:18865:1996 1:N:0:1\nGCTACAAAGATAATGATAGCGCTTATAGTTAGCTCGGTTTGCTAATTCGGTTTGACTAATATTATCCTTTAAATAATCTGTTTTCGCTTTTTTAGTCGGCGAAGGGGAACAAGCAAATAAAGTTGTCAGTGTTGAAATTAAAAAAATAAGT\n+\n>3A3>4@4C4FFBGFGGGGGGGGGGHGHHHHGGHHGGFGGGHHGHHHHGHGGGGEHHGGHHHHHHFHHHHFFHHHFHHHHHHHHHGGGHGHHGHGHHDCEGCEG?EGGGGHHHHHHHHHHHGHHHHGHHHHHHHHFFGHHEHHHGGHHHHH\n@M03610:15:000000000-ALE1U:1:1101:18806:2006 1:N:0:1\nGCGGATGGCTCGCCAAAAGTTGGCTCTTTACGAAAATTATTTTGTGACATTTAACTTTCCAATTTTAGGTGAAAATATAATGGGAAATTTCCCGCTGAATAAATTATCCCCATTTTAAAGTATTTCTCTTTAATCAGCACACGATTTTTCA\n+\n>>111>1AFFFDCEECGGGGGGHHHHHHHHHGFGC?GHFHHHHGGHHHHHHHHHHHHHHHHGHHHHHHGHHFHHHHHHHHGHGHHHHHHHBHHGGGGGHHHHHHHHHGHHGHHHHHHHHHEGHGHHHHGHHHHHHHFGGHGHGGHHGHHF2\n";
    const std::string readTabData = "TESTOne\tACTG\t####\tACTG\t####\nTestTwo\tACTG\t####\n";
};
    
TEST_F(ReadsTest, parseTabRead) {
    std::istringstream in1(readTabData);
    InputReader<ReadBase, TabReadImpl> ifs(in1);
    size_t read_count = 0;
    while(ifs.has_next()) {
        auto r = ifs.next();
        read_count++;
    }
    ASSERT_EQ(read_count, 2);
}

TEST_F(ReadsTest, parseSingleReadFastq) {
    std::istringstream in1(readData);
    InputReader<SingleEndRead, SingleEndReadFastqImpl> ifs(in1);
    size_t read_count = 0;
    while(ifs.has_next()) {
        auto r = ifs.next();
        std::cout << r->get_read().get_id() << std::endl;
        read_count++;
    }
    ASSERT_EQ(read_count, 5);
}    

TEST_F(ReadsTest, parsePairedReadFastq) {
    std::istringstream in1(readData);
    std::istringstream in2(readData);
    size_t read_count = 0;
    
    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(in1, in2);
    while(ifp.has_next()) {
        auto i = ifp.next();
        std::cout << i->get_read_one().get_qual() << std::endl;
        read_count++;
    }
    ASSERT_EQ(read_count, 5);
}
    
TEST_F(ReadsTest, testWriteFastqSingle) {
    std::istringstream in1(readData);
    InputReader<SingleEndRead, SingleEndReadFastqImpl> ifs(in1);

    std::ostringstream out1;
    {
        OutputWriter<SingleEndRead, SingleEndReadOutFastq> ofs(out1);

        while(ifs.has_next()) {
            auto r = ifs.next();
            ofs.write(*r);
        }
    }
    ASSERT_EQ(readData, out1.str());
}

TEST_F(ReadsTest, testTabWrite) {
    std::istringstream in1(readTabData);
    InputReader<ReadBase, TabReadImpl> ifs(in1);
    
    std::ostringstream out1;
    {
        OutputWriter<ReadBase, ReadBaseOutTab> ofs(out1);

        while(ifs.has_next()) {
            auto r = ifs.next();
            ofs.write(*r);
        }
    }
    ASSERT_EQ(readData, out1.str());
}
