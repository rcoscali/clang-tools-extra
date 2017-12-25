/**
 * @file FileManipulator.h
 *
 * @brief 
 * Class used to handle a file by either line number, byte offset, or other
 */

#ifndef _FILEMANIPULATOR_H_
#define _FILEMANIPULATOR_H_

#include <iostream>
#include <fstream>
#include <ios>
#include <string>
#include <map>

namespace jayacode
{

  class FileManipulator : public std::fstream
  {
  public:
    // Default constructor
    FileManipulator(void);

    // Explicit constructor
    explicit FileManipulator(const char* ,
                             std::ios_base::openmode mode = ios_base::in|ios_base::out);

    // Desctructor
    virtual ~FileManipulator(void);

    void create_line_number_mapping(void);
    void reset_line_number_mapping(void);

    std::string& operator[](unsigned int);
    void set_line(unsigned int, const std::string&);

    unsigned int get_number_of_lines(void);
    std::streamsize size(void) {return _size;};

  private:

    std::map<unsigned int, std::string> *line_number_mapping;
    std::streamsize _size;
    
  };
  
} // ! namespace jayacode

#endif /* ! _FILEMANIPULATOR_H_ */
