//===------- buffer_split.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <fstream>
#include <cstdlib>

#include "buffer_split.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        const std::string
        BufferSplitTest::LLVM_SRC_ROOT_DIR_ENVVAR_NAME = "LLVM_SRC_ROOT_DIR";

        const std::string
        BufferSplitTest::CLANG_TIDY_TEST_FILE_RELATIVE_PATH = "/tools/clang/tools/extra/clang-tidy/pagesjaunes/test/";

        const std::string
        BufferSplitTest::CLANG_TIDY_TEST_FILE_NAME = "buffer_split_std.txt";
        
        const std::string
        BufferSplitTest::CLANG_TIDY_TEST_BIG_FILE_NAME = "buffer_split_cpp.txt";
        
        const unsigned int
        BufferSplitTest::SHA256::sha256_k[64] =
          {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
            0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
            0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
            0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
            0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
          };
        
        BufferSplitTest::BufferSplitTest()
        {
          m_clang_root_directory = new std::string(std::getenv("LLVM_SRC_ROOT_DIR"));
          assert(m_clang_root_directory && !m_clang_root_directory->empty());
        }

        BufferSplitTest::~BufferSplitTest()
        {
          delete m_clang_root_directory;
        }

        void
        BufferSplitTest::SetUp(void)
        {
          if (std::getenv(LLVM_SRC_ROOT_DIR_ENVVAR_NAME.c_str()) == nullptr)
            std::cerr << "Error: environment: The LLVM_SRC_ROOT_DIR is not set. Some of these tests need this variable's value for successfully running !\n";
        }
        
        void
        BufferSplitTest::TearDown(void)
        {
        }
        
        void
        BufferSplitTest::PrintTo(const BufferSplitTest& buffer_split, ::std::ostream* os)
        {
        }

        TEST_F(BufferSplitTest, NominalBufferSplit)
        {
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>("line0\nline1\n"), linesnr);
          EXPECT_EQ(linesnr, 2);
          if (linesnr == 2)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "line0");
              EXPECT_STREQ(linesbuf[1].c_str(), "line1");
            }
        }

        TEST_F(BufferSplitTest, NominalBufferSplitStartAt1)
        {
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>("line1\nline2\n"), linesnr, 0, false);
          EXPECT_EQ(linesnr, 3);
          if (linesnr == 3)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "");
              EXPECT_STREQ(linesbuf[1].c_str(), "line1");
              EXPECT_STREQ(linesbuf[2].c_str(), "line2");
            }
        }

        TEST_F(BufferSplitTest, EmptyBuffer)
        {
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>(""), linesnr);
          EXPECT_EQ(linesnr, 0);
        }

        TEST_F(BufferSplitTest, OneEmptyLineBuffer)
        {
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>("\n"), linesnr);
          EXPECT_EQ(linesnr, 1);
          if (linesnr == 1)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "");
            }
        }

        TEST_F(BufferSplitTest, OneEmptyLineBufferStartAt1)
        {
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>("\n"), linesnr, 0, false);
          EXPECT_EQ(linesnr, 2);
          if (linesnr == 2)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "");
              EXPECT_STREQ(linesbuf[1].c_str(), "");
            }
        }

        TEST_F(BufferSplitTest, OneLineWithNoCRBuffer)
        {
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>("line0"), linesnr);
          EXPECT_EQ(linesnr, 1);
          if (linesnr == 1)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "line0");
            }
        }

        TEST_F(BufferSplitTest, OneLineWithNoCRBufferStartAt1)
        {
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>("line0"), linesnr, 0, false);
          EXPECT_EQ(linesnr, 2);
          if (linesnr == 2)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "");
              EXPECT_STREQ(linesbuf[1].c_str(), "line0");
            }
        }

        TEST_F(BufferSplitTest, BigBuffers)
        {
#include "buffer_split.test.h"
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>(bigbuf), linesnr, 28630, false);
          EXPECT_EQ(linesnr, 28626);
          if (linesnr == 28626)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "");
              EXPECT_STREQ(linesbuf[1].c_str(), "/*-------------------------------Identification-------------------------------*/");
              EXPECT_STREQ(linesbuf[28616].c_str(), "        doc2->value.a_classer.iNbPart = GIViNbTupleIapart;");
              EXPECT_STREQ(linesbuf[28625].c_str(), "}");
            }
        }

        TEST_F(BufferSplitTest, BigBuffersStartAt0)
        {
#include "buffer_split.test.h"
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>(bigbuf), linesnr, 28630, true);
          EXPECT_EQ(linesnr, 28625);
          if (linesnr == 28625)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "/*-------------------------------Identification-------------------------------*/");
              EXPECT_STREQ(linesbuf[28615].c_str(), "        doc2->value.a_classer.iNbPart = GIViNbTupleIapart;");
              EXPECT_STREQ(linesbuf[28624].c_str(), "}");
              EXPECT_STREQ(linesbuf[28625].c_str(), NULL);
            }
        }

        TEST_F(BufferSplitTest, BigBuffers2)
        {
#include "buffer_split.test2.h"
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>(bigbuf2), linesnr, 2400, false);
          EXPECT_EQ(linesnr, 2399);
          if (linesnr == 2399)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "");
              EXPECT_STREQ(linesbuf[1].c_str(), "Vlan pied le dieu rang nuit tu vite. Prepare charger six faisait oui dut cousine. Tard il gare boue si sent mais. Tot fut dela leur long. La de lasser rachat closes. Ah petites facteur maudite pendant agacent du. Habilement nos assurances age vie consentiez bleuissent vit. ");
              EXPECT_STREQ(linesbuf[129].c_str(), "As en celui vieux abris etais bride soirs et. Train eux mur bas car adore arbre voici. Un prudence negation flottent cervelle ah reprises du du. Quarante humaines et je tacherai. Sa me porte outre crete robes senti un du. Ah recupera reparler mourants je he epandent il depeches pourquoi. Poussaient paraissent ah un ce inattendus on. Attardent tu miserable illumines et mystiques superieur. Boulevards eux son executeurs vif simplement eclatantes commandant caracolent. Ma rythme disant courbe se reunir polies un parler. ");
              EXPECT_STREQ(linesbuf[2398].c_str(), "Ebloui brunes ere voulez des centre polies lorsqu. Ere fit eclatantes une decharnees pic habilement renferment. Et en ma plutot je admire pareil mutuel voyons. Mon defiance appareil peu reposoir dimanche moi mal galopent. Hors dur tous peu avis ses venu. Mal geste jeune des sapin ces dut. Son titres peuple manque veilla mur. ");
            }
        }

        TEST_F(BufferSplitTest, BigBuffers2StartAt0)
        {
#include "buffer_split.test2.h"
          std::vector<std::string>::size_type linesnr;
          std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>(bigbuf2), linesnr, 2400, true);
          EXPECT_EQ(linesnr, 2398);
          if (linesnr == 2398)
            {
              EXPECT_STREQ(linesbuf[0].c_str(), "Vlan pied le dieu rang nuit tu vite. Prepare charger six faisait oui dut cousine. Tard il gare boue si sent mais. Tot fut dela leur long. La de lasser rachat closes. Ah petites facteur maudite pendant agacent du. Habilement nos assurances age vie consentiez bleuissent vit. ");
              EXPECT_STREQ(linesbuf[128].c_str(), "As en celui vieux abris etais bride soirs et. Train eux mur bas car adore arbre voici. Un prudence negation flottent cervelle ah reprises du du. Quarante humaines et je tacherai. Sa me porte outre crete robes senti un du. Ah recupera reparler mourants je he epandent il depeches pourquoi. Poussaient paraissent ah un ce inattendus on. Attardent tu miserable illumines et mystiques superieur. Boulevards eux son executeurs vif simplement eclatantes commandant caracolent. Ma rythme disant courbe se reunir polies un parler. ");
              EXPECT_STREQ(linesbuf[2397].c_str(), "Ebloui brunes ere voulez des centre polies lorsqu. Ere fit eclatantes une decharnees pic habilement renferment. Et en ma plutot je admire pareil mutuel voyons. Mon defiance appareil peu reposoir dimanche moi mal galopent. Hors dur tous peu avis ses venu. Mal geste jeune des sapin ces dut. Son titres peuple manque veilla mur. ");
            }
        }

        TEST_F(BufferSplitTest, ReadWriteSplittedBuffer)
        {
          if (std::getenv(LLVM_SRC_ROOT_DIR_ENVVAR_NAME.c_str()) == nullptr)
            {
              EXPECT_TRUE(false);
            }
          else
            {
              std::string pathname(m_clang_root_directory->c_str());
              pathname.append("/");
              pathname.append(CLANG_TIDY_TEST_FILE_RELATIVE_PATH);
              pathname.append(CLANG_TIDY_TEST_FILE_NAME);
              std::string pathname_dst("/tmp/");
              pathname_dst.append(CLANG_TIDY_TEST_FILE_NAME);
              pathname_dst.append(".copy");
              
              std::size_t filesize;
              const char *buffer = clang::tidy::pagesjaunes::readTextFile(pathname.c_str(), filesize);
              std::vector<std::string>::size_type linesnr;
              // Evaluate number of lines with file length divied by 4 (just a guess)
              std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>(buffer), linesnr, filesize/4, true);
              EXPECT_EQ(linesnr, 8);
              if (linesnr == 8)
                {
                  EXPECT_STREQ(linesbuf[0].c_str(), "this is a test file");
                  EXPECT_STREQ(linesbuf[1].c_str(), "that is not too big");
                  EXPECT_STREQ(linesbuf[2].c_str(), "only a few lines");
                  EXPECT_STREQ(linesbuf[3].c_str(), "of standard text");
                  EXPECT_STREQ(linesbuf[4].c_str(), "le texte contient aussi");
                  EXPECT_STREQ(linesbuf[5].c_str(), "quelques caractères accentués");
                  EXPECT_STREQ(linesbuf[6].c_str(), "qui sont codés sur deux octets.");
                  // This is not a line returned (linesnr == 8) hence shall be empty
                  EXPECT_STREQ(linesbuf[7].c_str(), "");
                  
                  std::ofstream dst(pathname_dst, std::ios::out | std::ios::trunc | std::ios::binary);
                  if (dst.is_open())
                    {
                      unsigned int nwlines = 0;
                      for (auto lbit = linesbuf.begin(); lbit != linesbuf.end() && nwlines < linesnr; ++lbit, ++nwlines)
                          if (nwlines < linesnr)
                            {
                              std::string line = *lbit;
                              dst.write(line.c_str(), line.length());
                              dst.write("\n", 1);
                            }
                    }
                  dst.close();

                  std::size_t filesize_src;
                  const char *buffer_src = clang::tidy::pagesjaunes::readTextFile(pathname.c_str(), filesize_src);
                  std::string buf_src(buffer_src);
                  delete buffer_src;

                  std::size_t filesize_copy;
                  const char *buffer_copy = clang::tidy::pagesjaunes::readTextFile(pathname_dst.c_str(), filesize_copy);
                  std::string buf_copy(buffer_copy);
                  delete buffer_copy;
                  
                  EXPECT_TRUE(sha256cmp(buf_src, buf_copy) == 0);
                }
            }
        }

        TEST_F(BufferSplitTest, ReadWriteSplittedBufferStartAt1)
        {
          if (std::getenv(LLVM_SRC_ROOT_DIR_ENVVAR_NAME.c_str()) == nullptr)
            {
              EXPECT_TRUE(false);
            }
          else
            {
              std::string pathname(m_clang_root_directory->c_str());
              pathname.append("/");
              pathname.append(CLANG_TIDY_TEST_FILE_RELATIVE_PATH);
              pathname.append(CLANG_TIDY_TEST_FILE_NAME);
              std::string pathname_dst("/tmp/");
              pathname_dst.append(CLANG_TIDY_TEST_FILE_NAME);
              pathname_dst.append(".copy");
              
              std::size_t filesize;
              const char *buffer = clang::tidy::pagesjaunes::readTextFile(pathname.c_str(), filesize);
              std::vector<std::string>::size_type linesnr;
              // Evaluate number of lines with file length divied by 4
              std::vector<std::string> linesbuf = bufferSplit(const_cast<char *>(buffer), linesnr, filesize/4, false);
              EXPECT_EQ(linesnr, 9);
              if (linesnr == 9)
                {
                  EXPECT_STREQ(linesbuf[0].c_str(), "");
                  EXPECT_STREQ(linesbuf[1].c_str(), "this is a test file");
                  EXPECT_STREQ(linesbuf[2].c_str(), "that is not too big");
                  EXPECT_STREQ(linesbuf[3].c_str(), "only a few lines");
                  EXPECT_STREQ(linesbuf[4].c_str(), "of standard text");
                  EXPECT_STREQ(linesbuf[5].c_str(), "le texte contient aussi");
                  EXPECT_STREQ(linesbuf[6].c_str(), "quelques caractères accentués");
                  EXPECT_STREQ(linesbuf[7].c_str(), "qui sont codés sur deux octets.");
                  // This is not a line returned (linesnr == 8) hence shall be empty
                  EXPECT_STREQ(linesbuf[8].c_str(), "");
                  
                  std::ofstream dst(pathname_dst, std::ios::out | std::ios::trunc | std::ios::binary);
                  if (dst.is_open())
                    {
                      unsigned int nwlines = 0;
                      for (auto lbit = linesbuf.begin(); lbit != linesbuf.end() && nwlines < linesnr; ++lbit, ++nwlines)
                          if (nwlines > 0 && nwlines <= linesnr)
                            {
                              std::string line = *lbit;
                              dst.write(line.c_str(), line.length());
                              dst.write("\n", 1);
                            }
                    }
                  dst.close();

                  std::size_t filesize_src;
                  const char *buffer_src = clang::tidy::pagesjaunes::readTextFile(pathname.c_str(), filesize_src);
                  std::string buf_src(buffer_src);
                  delete buffer_src;

                  std::size_t filesize_copy;
                  const char *buffer_copy = clang::tidy::pagesjaunes::readTextFile(pathname_dst.c_str(), filesize_copy);
                  std::string buf_copy(buffer_copy);
                  delete buffer_copy;
                  
                  EXPECT_TRUE(sha256cmp(buf_src, buf_copy) == 0);
                }
            }
        }

        void
        BufferSplitTest::SHA256::transform(const unsigned char *message, unsigned int block_nb)
        {
          uint32 w[64];
          uint32 wv[8];
          uint32 t1, t2;
          const unsigned char *sub_block;
          int i;
          int j;
          for (i = 0; i < (int) block_nb; i++) {
            sub_block = message + (i << 6);
            for (j = 0; j < 16; j++) {
              SHA2_PACK32(&sub_block[j << 2], &w[j]);
            }
            for (j = 16; j < 64; j++) {
              w[j] =  SHA256_F4(w[j -  2]) + w[j -  7] + SHA256_F3(w[j - 15]) + w[j - 16];
            }
            for (j = 0; j < 8; j++) {
              wv[j] = m_h[j];
            }
            for (j = 0; j < 64; j++) {
              t1 = wv[7] + SHA256_F2(wv[4]) + SHA2_CH(wv[4], wv[5], wv[6])
                + sha256_k[j] + w[j];
              t2 = SHA256_F1(wv[0]) + SHA2_MAJ(wv[0], wv[1], wv[2]);
              wv[7] = wv[6];
              wv[6] = wv[5];
              wv[5] = wv[4];
              wv[4] = wv[3] + t1;
              wv[3] = wv[2];
              wv[2] = wv[1];
              wv[1] = wv[0];
              wv[0] = t1 + t2;
            }
            for (j = 0; j < 8; j++) {
              m_h[j] += wv[j];
            }
          }
        }
        
        void
        BufferSplitTest::SHA256::init()
        {
          m_h[0] = 0x6a09e667;
          m_h[1] = 0xbb67ae85;
          m_h[2] = 0x3c6ef372;
          m_h[3] = 0xa54ff53a;
          m_h[4] = 0x510e527f;
          m_h[5] = 0x9b05688c;
          m_h[6] = 0x1f83d9ab;
          m_h[7] = 0x5be0cd19;
          m_len = 0;
          m_tot_len = 0;
        }
        
        void
        BufferSplitTest::SHA256::update(const unsigned char *message, unsigned int len)
        {
          unsigned int block_nb;
          unsigned int new_len, rem_len, tmp_len;
          const unsigned char *shifted_message;
          tmp_len = BufferSplitTest::SHA256::SHA224_256_BLOCK_SIZE - m_len;
          rem_len = len < tmp_len ? len : tmp_len;
          memcpy(&m_block[m_len], message, rem_len);
          if (m_len + len < BufferSplitTest::SHA256::SHA224_256_BLOCK_SIZE) {
            m_len += len;
            return;
          }
          new_len = len - rem_len;
          block_nb = new_len / BufferSplitTest::SHA256::SHA224_256_BLOCK_SIZE;
          shifted_message = message + rem_len;
          transform(m_block, 1);
          transform(shifted_message, block_nb);
          rem_len = new_len % BufferSplitTest::SHA256::SHA224_256_BLOCK_SIZE;
          memcpy(m_block, &shifted_message[block_nb << 6], rem_len);
          m_len = rem_len;
          m_tot_len += (block_nb + 1) << 6;
        }
        
        void
        BufferSplitTest::SHA256::final(unsigned char *digest)
        {
          unsigned int block_nb;
          unsigned int pm_len;
          unsigned int len_b;
          int i;

          block_nb = (1 + ((BufferSplitTest::SHA256::SHA224_256_BLOCK_SIZE - 9)
                           < (m_len % BufferSplitTest::SHA256::SHA224_256_BLOCK_SIZE)));
          len_b = (m_tot_len + m_len) << 3;
          pm_len = block_nb << 6;
          memset(m_block + m_len, 0, pm_len - m_len);
          m_block[m_len] = 0x80;
          SHA2_UNPACK32(len_b, m_block + pm_len - 4);
          transform(m_block, block_nb);
          for (i = 0 ; i < 8; i++)
            SHA2_UNPACK32(m_h[i], &digest[i << 2]);
        }
        
        std::string
        BufferSplitTest::sha256(const std::string& input)
        {
          unsigned char digest[BufferSplitTest::SHA256::DIGEST_SIZE];
          memset(digest, 0, BufferSplitTest::SHA256::DIGEST_SIZE);
          
          BufferSplitTest::SHA256 ctx = BufferSplitTest::SHA256();
          ctx.init();
          ctx.update(reinterpret_cast<const unsigned char *>(input.c_str()), input.length());
          ctx.final(digest);
          
          char buf[2 * BufferSplitTest::SHA256::DIGEST_SIZE+1];
          buf[2 * BufferSplitTest::SHA256::DIGEST_SIZE] = 0;
          for (unsigned int i = 0; i < BufferSplitTest::SHA256::DIGEST_SIZE; i++)
            sprintf(buf+i*2, "%02x", digest[i]);

          return std::string(buf);
        }
        
        std::string
        BufferSplitTest::sha256(std::ifstream& is)
        {
          std::streambuf *pbuf = is.rdbuf();
          std::streamsize filelen = pbuf->pubseekoff(0, is.end);
          pbuf->pubseekoff(0, is.beg);
          char *buffer = new char[filelen];
          pbuf->sgetn(buffer, filelen);
          std::string stringbuf(buffer);
          delete buffer;
          
          return sha256(stringbuf);
        }

        int
        BufferSplitTest::sha256cmp(const std::string& s1, const std::string& s2)
        {
          std::string sha256s1 = BufferSplitTest::sha256(s1);
          std::string sha256s2 = BufferSplitTest::sha256(s2);
          return sha256s1.compare(sha256s2);
        }
        
        int
        BufferSplitTest::sha256cmp(std::ifstream& is1, std::ifstream& is2)
        {
          std::string sha256s1 = BufferSplitTest::sha256(is1);
          std::string sha256s2 = BufferSplitTest::sha256(is2);
          return sha256s1.compare(sha256s2);
        }
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
