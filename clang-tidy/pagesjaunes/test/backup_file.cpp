//===------- backup_file.cpp - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdio.h>     /* remove */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <iostream>
#include <fstream>

#include "backup_file.h"

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {
        
        const unsigned int
        BackupFile::SHA256::sha256_k[64] =
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
        
        BackupFile::BackupFile()
        {
          srand (time(NULL));

          // File between 1 and 2 megabytes
          m_length = ONEMEGA + (rand() % ONEMEGA + 1);          
          m_buffer = new unsigned char [m_length];
          assert(m_buffer);

          // Entry file is random
          std::ifstream src("/dev/urandom", std::ios::binary);
          src.read(reinterpret_cast<char *>(m_buffer), m_length);
          src.close();
          m_sha256_value = sha256(std::string(reinterpret_cast<char *>(m_buffer)));
        }

        BackupFile::~BackupFile()
        {
          delete m_buffer;
        }

        void
        BackupFile::SetUp(void)
        {
          SetUpSimpleBackup();
          SetUpSimpleBackup0();
          SetUpSimpleBackup1();
          SetUpManyBackup();
        }
        
        void
        BackupFile::SetUpSimpleBackup(void)
        {
          // File for test SimpleBackup, SimpleBackup.test, we test /tmp/SimpleBackup.test.bak
          std::ofstream dst_1("/tmp/SimpleBackup.test", std::ios::binary);
          dst_1.write(reinterpret_cast<char *>(m_buffer), m_length);
          dst_1.close();
        }
        
        void
        BackupFile::SetUpSimpleBackup0(void)
        {
          // File for test SimpleBackup0, SimpleBackup0.test, we test /tmp/SimpleBackup0.test.bak0
          std::ofstream dst0_1("/tmp/SimpleBackup0.test", std::ios::binary);
          dst0_1.write(reinterpret_cast<char *>(m_buffer), m_length);
          dst0_1.close();
          std::ofstream dst0_2("/tmp/SimpleBackup0.test.bak", std::ios::binary);
          dst0_2.write(reinterpret_cast<char *>(m_buffer), m_length);
          dst0_2.close();
        }
        
        void
        BackupFile::SetUpSimpleBackup1(void)
        {
          // File for test SimpleBackup1, SimpleBackup1.test, we test /tmp/SimpleBackup1.test.bak1
          std::ofstream dst1_1("/tmp/SimpleBackup1.test", std::ios::binary);
          dst1_1.write(reinterpret_cast<char *>(m_buffer), m_length);
          dst1_1.close();
          std::ofstream dst1_2("/tmp/SimpleBackup1.test.bak", std::ios::binary);
          dst1_2.write(reinterpret_cast<char *>(m_buffer), m_length);
          dst1_2.close();
          std::ofstream dst1_3("/tmp/SimpleBackup1.test-0.bak", std::ios::binary);
          dst1_3.write(reinterpret_cast<char *>(m_buffer), m_length);
          dst1_3.close();
        }
        
        void
        BackupFile::SetUpManyBackup(void)
        {
          // File for test SimpleBackup, SimpleBackup.test, we test /tmp/SimpleBackup.test.bak
          std::ofstream dst_1("/tmp/ManyBackup.test", std::ios::binary);
          dst_1.write(reinterpret_cast<char *>(m_buffer), 1024);
          dst_1.close();
        }
        
        void
        BackupFile::TearDown(void)
        {
          remove("/tmp/SimpleBackup.test");
          remove("/tmp/SimpleBackup.test.bak");
          remove("/tmp/SimpleBackup0.test");
          remove("/tmp/SimpleBackup0.test.bak");
          remove("/tmp/SimpleBackup0.test-0.bak");
          remove("/tmp/SimpleBackup1.test");
          remove("/tmp/SimpleBackup1.test.bak");
          remove("/tmp/SimpleBackup1.test-0.bak");
          remove("/tmp/SimpleBackup1.test-1.bak");
          (void)system("/bin/rm /tmp/ManyBackup.test*");
        }
        
        void
        BackupFile::PrintTo(const BackupFile& backup_file, ::std::ostream* os)
        {
        }
 
        
        TEST_F(BackupFile, SimpleBackup)
        {
          clang::tidy::pagesjaunes::createBackupFile("/tmp/SimpleBackup.test");
          std::ifstream src("/tmp/SimpleBackup.test.bak", std::ios::binary);
          char *buffer = new char[m_length];
          assert(buffer);
          src.rdbuf()->sgetn(buffer, m_length);
          std::string hashed = sha256(std::string(buffer));
          EXPECT_STREQ(m_sha256_value.c_str(), hashed.c_str());
          delete buffer;
        }
        
        TEST_F(BackupFile, SimpleBackup0)
        {
          clang::tidy::pagesjaunes::createBackupFile("/tmp/SimpleBackup0.test");
          std::ifstream src("/tmp/SimpleBackup0.test-0.bak", std::ios::binary);
          char *buffer = new char[m_length];
          assert(buffer);
          src.rdbuf()->sgetn(buffer, m_length);
          std::string hashed = sha256(std::string(buffer));
          EXPECT_STREQ(m_sha256_value.c_str(), hashed.c_str());
          delete buffer;
        }
        
        TEST_F(BackupFile, SimpleBackup1)
        {
          clang::tidy::pagesjaunes::createBackupFile("/tmp/SimpleBackup1.test");
          std::ifstream src("/tmp/SimpleBackup1.test-1.bak", std::ios::binary);
          char *buffer = new char[m_length];
          assert(buffer);
          src.rdbuf()->sgetn(buffer, m_length);
          std::string hashed = sha256(std::string(buffer));
          EXPECT_STREQ(m_sha256_value.c_str(), hashed.c_str());
          delete buffer;
        }

        TEST_F(BackupFile, ManyBackupLog2)
        {
          for(unsigned int n = 0; n < 26; n++)
            {
              clang::tidy::pagesjaunes::createBackupFile("/tmp/ManyBackup.test");
            }

          std::ifstream src("/tmp/SimpleBackup.test", std::ios::binary);
          char *buffer = new char[1024];
          assert(buffer);
          src.rdbuf()->sgetn(buffer, 1024);
          std::string expected_hashed = sha256(std::string(buffer));
          src.close();

          std::ifstream bak("/tmp/SimpleBackup.test-24.bak", std::ios::binary);
          bak.rdbuf()->sgetn(buffer, 1024);
          std::string hashed = sha256(std::string(buffer));
          EXPECT_STREQ(expected_hashed.c_str(), hashed.c_str());
          bak.close();

          delete buffer;
        }
        
        TEST_F(BackupFile, ManyBackupLog3)
        {
          for(unsigned int n = 0; n < 202; n++)
            {
              clang::tidy::pagesjaunes::createBackupFile("/tmp/ManyBackup.test");
            }

          std::ifstream src("/tmp/SimpleBackup.test", std::ios::binary);
          char *buffer = new char[1024];
          assert(buffer);
          src.rdbuf()->sgetn(buffer, 1024);
          std::string expected_hashed = sha256(std::string(buffer));
          src.close();

          std::ifstream bak("/tmp/SimpleBackup.test-200.bak", std::ios::binary);
          bak.rdbuf()->sgetn(buffer, 1024);
          std::string hashed = sha256(std::string(buffer));
          EXPECT_STREQ(expected_hashed.c_str(), hashed.c_str());
          bak.close();

          delete buffer;
        }
        
        void
        BackupFile::SHA256::transform(const unsigned char *message, unsigned int block_nb)
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
        BackupFile::SHA256::init()
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
        BackupFile::SHA256::update(const unsigned char *message, unsigned int len)
        {
          unsigned int block_nb;
          unsigned int new_len, rem_len, tmp_len;
          const unsigned char *shifted_message;
          tmp_len = BackupFile::SHA256::SHA224_256_BLOCK_SIZE - m_len;
          rem_len = len < tmp_len ? len : tmp_len;
          memcpy(&m_block[m_len], message, rem_len);
          if (m_len + len < BackupFile::SHA256::SHA224_256_BLOCK_SIZE) {
            m_len += len;
            return;
          }
          new_len = len - rem_len;
          block_nb = new_len / BackupFile::SHA256::SHA224_256_BLOCK_SIZE;
          shifted_message = message + rem_len;
          transform(m_block, 1);
          transform(shifted_message, block_nb);
          rem_len = new_len % BackupFile::SHA256::SHA224_256_BLOCK_SIZE;
          memcpy(m_block, &shifted_message[block_nb << 6], rem_len);
          m_len = rem_len;
          m_tot_len += (block_nb + 1) << 6;
        }
        
        void
        BackupFile::SHA256::final(unsigned char *digest)
        {
          unsigned int block_nb;
          unsigned int pm_len;
          unsigned int len_b;
          int i;

          block_nb = (1 + ((BackupFile::SHA256::SHA224_256_BLOCK_SIZE - 9)
                           < (m_len % BackupFile::SHA256::SHA224_256_BLOCK_SIZE)));
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
        BackupFile::sha256(const std::string& input)
        {
          unsigned char digest[BackupFile::SHA256::DIGEST_SIZE];
          memset(digest, 0, BackupFile::SHA256::DIGEST_SIZE);
          
          BackupFile::SHA256 ctx = BackupFile::SHA256();
          ctx.init();
          ctx.update(reinterpret_cast<const unsigned char *>(input.c_str()), input.length());
          ctx.final(digest);
          
          char buf[2 * BackupFile::SHA256::DIGEST_SIZE+1];
          buf[2 * BackupFile::SHA256::DIGEST_SIZE] = 0;
          for (unsigned int i = 0; i < BackupFile::SHA256::DIGEST_SIZE; i++)
            sprintf(buf+i*2, "%02x", digest[i]);
          return std::string(buf);
        }
        
        std::string
        BackupFile::sha256(std::ifstream& is)
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
        BackupFile::sha256cmp(const std::string& s1, const std::string& s2)
        {
          std::string sha256s1 = BackupFile::sha256(s1);
          std::string sha256s2 = BackupFile::sha256(s2);
          return sha256s1.compare(sha256s2);
        }
        
        int
        BackupFile::sha256cmp(std::ifstream& is1, std::ifstream& is2)
        {
          std::string sha256s1 = BackupFile::sha256(is1);
          std::string sha256s2 = BackupFile::sha256(is2);
          return sha256s1.compare(sha256s2);
        }
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang
