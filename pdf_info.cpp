#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <cstdarg>
#include <algorithm>

#include "PDFParser.h"
#include "PDFObjectCast.h"
#include "PDFDictionary.h"
#include "PDFDocumentCopyingContext.h"
#include "InputFile.h"
#include "RefCountPtr.h"
#include "PDFIndirectObjectReference.h"
#include "PDFStreamInput.h"
#include "IByteReader.h"
#include "EStatusCode.h"
#include "ParsedPrimitiveHelper.h"
#include "PDFInteger.h"
#include "PDFArray.h"
#include "PDFName.h"
#include "PDFReal.h"
#include "PDFBoolean.h"
#include "PDFTextString.h"
#include "PDFHexString.h"
#include "PDFLiteralString.h"
#include "UnicodeString.h"

#define ENUM2STR1(a,id,s) (a == id) ? s :
#define ENUM2STR(a,id) ENUM2STR1(a,id,#id)

#define PDFDICTIONARY_TO_STR(a) (\
  ENUM2STR(a,PDFDictionary::ePDFObjectBoolean) \
  ENUM2STR(a,PDFDictionary::ePDFObjectLiteralString) \
  ENUM2STR(a,PDFDictionary::ePDFObjectHexString) \
  ENUM2STR(a,PDFDictionary::ePDFObjectNull) \
  ENUM2STR(a,PDFDictionary::ePDFObjectName) \
  ENUM2STR(a,PDFDictionary::ePDFObjectInteger) \
  ENUM2STR(a,PDFDictionary::ePDFObjectReal) \
  ENUM2STR(a,PDFDictionary::ePDFObjectArray) \
  ENUM2STR(a,PDFDictionary::ePDFObjectDictionary) \
  ENUM2STR(a,PDFDictionary::ePDFObjectIndirectObjectReference) \
  ENUM2STR(a,PDFDictionary::ePDFObjectStream) \
  ENUM2STR(a,PDFDictionary::ePDFObjectSymbol) \
  "UNKNOWN" \
)

static const std::string _NULL_ = "__MAGIC_NULL__";

std::string string_format(const char* fmt...) {
  try {
    std::string ret;
    std::va_list args;
    char* buf;
    int length;
    va_start(args, fmt);
    length = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    buf = new char[length + 1];
    buf[length] = 0;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    ret = buf;
    delete[] buf;
    return ret;
  } catch(std::bad_alloc& ba) {
    return _NULL_;
  }
}

std::string& string_replace(std::string& s, std::string const& to_find, std::string const& replace_with, ssize_t limit = 0) {
  std::string buf;
  size_t pos = 0;
  size_t prev_pos = 0;
  size_t count = 0;

  // Reserves rough estimate of final size of string.
  buf.reserve(s.size());
  while(true) {
    prev_pos = pos;
    pos = s.find(to_find, pos);
    if(limit > 0 && limit == count) {
      break;
    }
    if(pos == std::string::npos) {
      break;
    }
    buf.append(s, prev_pos, pos - prev_pos);
    buf += replace_with;
    pos += to_find.size();
    count++;
  }

  buf.append(s, prev_pos, s.size() - prev_pos);
  s.swap(buf);
  return s;
}

void showPDFinfo(PDFParser& parser) {
  printf("\nPDF Header level = %f\n", parser.GetPDFLevel());
  printf("Number of objects in PDF = %lu\n", parser.GetObjectsCount());
  printf("Number of pages in PDF = %lu\n", parser.GetPagesCount());
};

std::string pdfobject_type(PDFObject* value) {
  std::string ret;
  ret = string_format("%s", PDFDICTIONARY_TO_STR(value->GetType()));
  string_replace(ret, "PDFDictionary::", "");
  return ret;
}

void print_pdfobject(std::vector<ObjectIDType> &indirects, PDFParser& parser, PDFObject* value, uint64_t depth = 0, bool dry_run = false) {
  if(!dry_run) {
    printf(string_format("%%-%lus", depth * 2).c_str(), "");
    printf("%s: ", pdfobject_type(value).c_str());
    fflush(stdout);
  }

  switch(value->GetType()) {
    case PDFDictionary::ePDFObjectArray: {
      PDFObjectCastPtr<PDFArray> _value = value;
      PDFObject* __value;
      if(!dry_run) {
        printf("\n");
      }
      for(unsigned long i = 0; i < _value->GetLength(); i++) {
        __value = parser.QueryArrayObject(_value.GetPtr(), i);
        switch(__value->GetType()) {
          case PDFDictionary::ePDFObjectArray:
          case PDFDictionary::ePDFObjectDictionary: {
            print_pdfobject(indirects, parser, __value, depth + 1, dry_run);
            break;
          }
          case PDFDictionary::ePDFObjectBoolean:
          case PDFDictionary::ePDFObjectLiteralString:
          case PDFDictionary::ePDFObjectHexString:
          case PDFDictionary::ePDFObjectNull:
          case PDFDictionary::ePDFObjectName:
          case PDFDictionary::ePDFObjectInteger:
          case PDFDictionary::ePDFObjectReal:
          case PDFDictionary::ePDFObjectIndirectObjectReference:
          case PDFDictionary::ePDFObjectStream:
          case PDFDictionary::ePDFObjectSymbol: {
            print_pdfobject(indirects, parser, __value, depth + 1, dry_run);
            break;
          }
        }
      }
      break;
    }
    case PDFDictionary::ePDFObjectDictionary: {
      PDFObjectCastPtr<PDFDictionary> _value = value;
      if(!_value.GetPtr()) {
        break;
      }

      MapIterator<PDFNameToPDFObjectMap> it = _value->GetIterator();
      PDFName* _key;
      PDFObject* __value;
      if(!dry_run) {
        printf("\n");
      }
      while(it.MoveNext()) {
        _key = it.GetKey();
        __value = it.GetValue();
        if(!dry_run) {
          printf(string_format("%%-%lus", (depth + 1) * 2).c_str(), "");
          fflush(stdout);
          printf("key = %s\n", _key->GetValue().c_str());
        }
        print_pdfobject(indirects, parser, __value, depth + 2, dry_run);
      }
      break;
    }
    case PDFDictionary::ePDFObjectBoolean: {
      if(!dry_run) {
        printf("value = %s\n", ((PDFObjectCastPtr<PDFBoolean>)value)->GetValue() ? "true" : "false");
      }
      break;
    }
    case PDFDictionary::ePDFObjectLiteralString: {
      if(!dry_run) {
        printf("value = %s\n", ((PDFObjectCastPtr<PDFLiteralString>)value)->GetValue().c_str());
      }
      break;
    }
    case PDFDictionary::ePDFObjectNull: {
      if(!dry_run) {
        printf("value = NULL\n");
      }
      break;
    }
    case PDFDictionary::ePDFObjectName: {
      if(!dry_run) {
        printf("value = %s\n", ((PDFObjectCastPtr<PDFName>)value)->GetValue().c_str());
      }
      break;
    }
    case PDFDictionary::ePDFObjectInteger: {
      if(!dry_run) {
        printf("value = %lli\n", ((PDFObjectCastPtr<PDFInteger>)value)->GetValue());
      }
      break;
    }
    case PDFDictionary::ePDFObjectReal: {
      if(!dry_run) {
        printf("value = %f\n", ((PDFObjectCastPtr<PDFReal>)value)->GetValue());
      }
      break;
    }
    case PDFDictionary::ePDFObjectStream: {
      PDFObjectCastPtr<PDFStreamInput> _value = value;
      if(!dry_run) {
        Byte c;
        IByteReader* stream = parser.StartReadingFromStream(_value.GetPtr());
        std::string prefix = string_format("%%-%lus", (depth + 1) * 2);
        bool is_hex = false;
        while(stream->NotEnded()) {
          IOBasicTypes::LongBufferSizeType read;
          if((read = stream->Read(&c, 1)) != 1) {
            break;
          }
          if(!isascii(c)) {
            is_hex = true;
            break;
          }
        }

        int pos = 0;
        printf("\n");
        printf(prefix.c_str(), "");
        stream = parser.StartReadingFromStream(_value.GetPtr());

        //is_hex = true; //force hex style
        while(stream->NotEnded()) {
          IOBasicTypes::LongBufferSizeType read;
          if(read = stream->Read(&c, 1) != 1) {
            break;
          }
          if(is_hex) {
            printf("%02X", c & 0x0FF);

            if(((pos + 1) % 16) == 0) {
              printf("\n");
              printf(prefix.c_str(), "");
              pos = 0;
            } else {
              printf(" ");
              pos++;
            }
          } else {
            printf("%c", c & 0x0FF);
          }
        }
        printf("\n");
      }
      break;
    }

    case PDFDictionary::ePDFObjectIndirectObjectReference: {
      PDFObjectCastPtr<PDFIndirectObjectReference> _value = value;
      if(!dry_run) {
        printf("value = %lu\n", _value->mObjectID);
      }
      if(std::find(indirects.begin(), indirects.end(), _value->mObjectID) != indirects.end()) {
        break;
      }
      indirects.push_back(_value->mObjectID);
      break;
    }

    case PDFDictionary::ePDFObjectHexString: {
      PDFObjectCastPtr<PDFHexString> _value = value;
      std::string v = _value->GetValue();
      if(!dry_run) {
        printf("value = ");
        for(size_t i = 2; i < v.size(); i += 2) {
          printf("%c", v.at(i + 1));
        }
        printf("\n");
      }
      break;
    }

    case PDFDictionary::ePDFObjectSymbol: {
      if(!dry_run) {
        printf("value = UNKNOWN\n");
      }
      break;
    }

    default: {
      break;
    }
  }
}

void print_indirects(PDFParser& parser, std::vector<ObjectIDType>& indirects) {
  std::vector<ObjectIDType> t_indirects;

  t_indirects = indirects;
  do {
    for(size_t i = 0; i < indirects.size(); i++) {
      PDFObject* __value = parser.ParseNewObject(indirects[i]);
      if(__value != NULL) {
        print_pdfobject(indirects, parser, __value,  1, true);
      }
    }
    t_indirects = indirects;
  } while(indirects.size() != t_indirects.size());

  std::sort(indirects.begin(), indirects.end());

  for(size_t i = 0; i < indirects.size(); i++) {
    printf("ePDFObjectIndirectObjectReference: Start : value = %lu\n", indirects[i]);
    PDFObject* __value = parser.ParseNewObject(indirects[i]);
    if(__value != NULL) {
      print_pdfobject(indirects, parser, __value,  1, false);
    }
    printf("ePDFObjectIndirectObjectReference: End   : value = %lu\n", indirects[i]);
    printf("\n");
  }
  t_indirects = indirects;
}

void showPagesInfo(PDFParser& parser, InputFile& pdfFile, EStatusCode status) {
  for(unsigned long i = 0; i < parser.GetPagesCount() && eSuccess == status; ++i) {
    printf("\n// Showing info for Page %lu //////////////////////////////////////////////////////////\n", i);
    printf("Showing info for page %lu:\n", i);
    std::vector<ObjectIDType> indirects;
    print_pdfobject(indirects, parser, parser.ParsePage(i), 0);
    printf("\n// Showing Indirect Object for Page %lu /////////////////////////////////////////////\n\n", i);
    print_indirects(parser, indirects);
    printf("///////////////////////////////////////////////////////////////////////////////////////\n");
  }
};

EStatusCode parsePDF(std::string pdf) {
  PDFParser parser;
  InputFile pdfFile;

  EStatusCode status = pdfFile.OpenFile(pdf);
  if(status != eSuccess) {
    return status;
  }

  status = parser.StartPDFParsing(pdfFile.GetInputStream());
  if(status != eSuccess) {
    return status;
  }

  showPDFinfo(parser); // Just wcout some info (no iteration)
  showPagesInfo(parser, pdfFile, status);
  return status;
};

int main(int argc, char** argv) {
  if(parsePDF(argv[1]) == eSuccess) {
    printf("Parsing succeeded\n");
    return 0;
  } else {
    printf("Parsing failed\n");
    return -1;
  }

  return 0;
}
