/**
 * @file FileManipulator.cpp
 *
 * @brief
 * Implem for FileManipulator class handling a file by line number, byte offset, char number etc
 *
 */

#include "FileManipulator.h"

using namespace std;

namespace jayacode
{

  /**
   * Default constructor
   */
  FileManipulator::FileManipulator(void)
    : fstream()
  {
    line_number_mapping = nullptr;
    _size = 0;
  }

  /**
   * Explicit constructor
   */
  FileManipulator::FileManipulator(const char *filename, ios_base::openmode mode)
    : fstream(filename, mode)
  {
    line_number_mapping = nullptr;
    _size = 0;
  }

  /**
   * Destructor
   */
  FileManipulator::~FileManipulator(void)
  {
  }

  /**
   *
   */
  std::string&
  FileManipulator::operator[](unsigned int linen)
  {
    if (!line_number_mapping)
      create_line_number_mapping();
    if (linen < 1 || linen > line_number_mapping->size())
      throw std::out_of_range("Line number out of range!");
    return((*line_number_mapping)[linen-1]);
  }
  
  /**
   *
   */
  void
  FileManipulator::set_line(unsigned int linen, const std::string &line)
  {
    if (linen < 1 || linen > line_number_mapping->size())
      throw std::out_of_range("Line number out of range!");
    line_number_mapping->erase(linen-1);
    line_number_mapping->emplace(linen-1, line);
  }
  
  /**
   *
   */
  unsigned int
  FileManipulator::get_number_of_lines(void)
  {
    if (is_open())
      {
        if (!line_number_mapping)
          create_line_number_mapping();
        return(line_number_mapping->size());
      }
    return(0);
  }
  
  /**
   *
   */
  void
  FileManipulator::create_line_number_mapping(void)
  {
    if (line_number_mapping == nullptr)
      {
        unsigned int linen = 0;
        line_number_mapping = new map<unsigned int, std::string>(); 
        char *contents, *p, *q;
        std::streambuf *sb = rdbuf();
        _size = sb->pubseekoff(0, end);
        sb->pubseekoff(0, beg);
        contents = new char [_size];
        sb->sgetn (contents, _size);
        sb->pubseekoff(0, beg);
        p = q = contents;
        
        do
          {
            // Go to the first CR
            while (*p && *p != '\n') p++;
            if (*p)
              {
                (*line_number_mapping)[linen] = std::string(q, p - q);
                linen++;
                p++;
                q = p;
              }
          }
        while (*p);
      }
  }
            
  /**
   *
   */
  void
  FileManipulator::reset_line_number_mapping(void)
  {
    if (line_number_mapping != nullptr)
      {
        delete line_number_mapping;
        line_number_mapping = nullptr;
      }
  }
  
} // ! namespace jayacode
