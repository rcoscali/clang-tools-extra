//===--- buffer_split.h - clang-tidy ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __BUFFER_SPLIT_H__
#define __BUFFER_SPLIT_H__

#include <fstream>
#include <vector>
#include <string>

#include "gtest/gtest.h"

#include "llvm/Support/Regex.h"
#include "ExecSQLCommon.h"

#define SHA2_SHFR(x, n)   (x >> n)
#define SHA2_ROTR(x, n)   ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define SHA2_ROTL(x, n)   ((x << n) | (x >> ((sizeof(x) << 3) - n)))
#define SHA2_CH(x, y, z)  ((x & y) ^ (~x & z))
#define SHA2_MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define SHA256_F1(x)      (SHA2_ROTR(x,  2) ^ SHA2_ROTR(x, 13) ^ SHA2_ROTR(x, 22))
#define SHA256_F2(x)      (SHA2_ROTR(x,  6) ^ SHA2_ROTR(x, 11) ^ SHA2_ROTR(x, 25))
#define SHA256_F3(x)      (SHA2_ROTR(x,  7) ^ SHA2_ROTR(x, 18) ^ SHA2_SHFR(x,  3))
#define SHA256_F4(x)      (SHA2_ROTR(x, 17) ^ SHA2_ROTR(x, 19) ^ SHA2_SHFR(x, 10))

#define SHA2_UNPACK32(x, str)                 \
{                                             \
    *((str) + 3) = (uint8) ((x)      );       \
    *((str) + 2) = (uint8) ((x) >>  8);       \
    *((str) + 1) = (uint8) ((x) >> 16);       \
    *((str) + 0) = (uint8) ((x) >> 24);       \
}

#define SHA2_PACK32(str, x)                   \
{                                             \
    *(x) =   ((uint32) *((str) + 3)      )    \
           | ((uint32) *((str) + 2) <<  8)    \
           | ((uint32) *((str) + 1) << 16)    \
           | ((uint32) *((str) + 0) << 24);   \
}          

namespace clang
{

  namespace tidy
  {

    namespace pagesjaunes
    {

      namespace test
      {

        class BufferSplitTest : public ::testing::Test
        {
        public:
          class SHA256
          {
          protected:
            typedef unsigned char uint8;
            typedef unsigned int uint32;
            typedef unsigned long long uint64;
            
            const static uint32 sha256_k[];
            static const unsigned int SHA224_256_BLOCK_SIZE = (512/8);

          public:
            void init();
            void update(const unsigned char *message, unsigned int len);
            void final(unsigned char *digest);
            static const unsigned int DIGEST_SIZE = (256 / 8);
            
          protected:
            void transform(const unsigned char *message, unsigned int block_nb);
            unsigned int m_tot_len;
            unsigned int m_len;
            unsigned char m_block[2*SHA224_256_BLOCK_SIZE];
            uint32 m_h[8];
          };

          const static std::string LLVM_SRC_ROOT_DIR_ENVVAR_NAME;
          const static std::string CLANG_TIDY_TEST_FILE_RELATIVE_PATH;
          const static std::string CLANG_TIDY_TEST_FILE_NAME;
          const static std::string CLANG_TIDY_TEST_BIG_FILE_NAME;

          BufferSplitTest();
          virtual ~BufferSplitTest();
          
          virtual void SetUp(void);
          virtual void TearDown(void);
          
          // It's important that PrintTo() is defined in the SAME
          // namespace that defines Bar.  C++'s look-up rules rely on that.
          virtual void PrintTo(const BufferSplitTest&, ::std::ostream*);
          
          std::string sha256(const std::string&);
          std::string sha256(std::ifstream&);
          int sha256cmp(const std::string&, const std::string&);
          int sha256cmp(std::ifstream&, std::ifstream&);

        protected:
          // The CLANG root directory read from LLVM_SRC_ROOT_DIR env var
          const std::string* m_clang_root_directory = nullptr;

        private:

        };
        
      } // ! namespace test
      
    } // ! namespace pagesjaunes

  } // ! namespace tidy
  
} // ! namespace clang

#endif /*! __BUFFER_SPLIT_H__ */
