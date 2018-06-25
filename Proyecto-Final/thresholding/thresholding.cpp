
#include "thresholding.h"

// LTI-Lib Headers

#include <ltiObject.h>
#include <ltiMath.h>     // General lti:: math and <cmath> functionality
#include <ltiTimer.h>    // To measure time

#include <ltiLispStreamHandler.h>

#include <ltiThresholding.h>

#include <ltiDraw.h>
#include <ltiViewer2D.h>
#include <ltiViewer1D.h>
#include <ltiImage.h>
#include <ltiIOImage.h>
#include <ltiColors.h>

// Standard Headers: from ANSI C and GNU C Library
#include <cstdlib>  // Standard Library for C++
#include <getopt.h> // Functions to parse the command line arguments

// Standard Headers: STL
#include <iostream>
#include <string>
#include <fstream>

// Debug

// Ensure that the STL streaming is used.
using std::cout;
using std::cerr;
using std::endl;

#undef _LTI_DEBUG
//#define _LTI_DEBUG 4
#include "ltiDebug.h"


thresh::thresh(int argc, char* argv[]) 
  : task_(None) {
  parse(argc,argv);
}


/*
 * Help 
 */
void thresh::usage() const {
  cout <<
    "usage: thresholding [options] <image> \n\n" \
    "       -f      Use gray-channel of floats\n" \
    "       -8      Use gray-channel of bytes\n" \
    "       <image>  input image" << std::endl; 
}

void thresh::help() const {
  cout << 
    " h      Select high threshold.\n" \
    " l      Select low threshold.\n" \
    " o      Otsu.\n" \
    " O      Otsu in interval.\n" \
    " s      Simple adaption.\n" \
    " S      Simple adaption in interval.\n" \
    " d      Direct threshold.\n" \
    " r      Relative threshold.\n" \
    " b      Toggle keep background\n" \
    " f      Toggle keep foreground\n" \
    " Arrows Increase/Decrease selected threshold.\n" \
    " ?      Print this message.\n" << std::endl;
}

/*
 * Parse the line command arguments
 */
void thresh::parse(int argc, char*argv[]) {

  int c;

  // We use the standard getopt.h functions here to parse the arguments.
  // Check the documentation of getopt.h for more information on this
  
  // structure for the long options. 
  static struct option lopts[] = {
    {"float",no_argument,0,'f'},
    {"byte",no_argument,0,'8'},
    {"help",no_argument,0,'h'},
    {0,0,0,0}
  };

  int optionIdx;

  while ((c = getopt_long(argc, argv, "f8h", lopts,&optionIdx)) != -1) {
    switch (c) {
    case 'f':
      task_=Float;
      break;
    case '8':
      task_=Byte;
      break;
    case 'h':
      usage();
      exit(EXIT_SUCCESS);
      break;
    default:
      cerr << "Option '-" << static_cast<char>(c) << "' not recognized." 
           << endl;
    }
  }
  
  if (optind < argc) {
    imgFile_ = argv[optind];
  }
  

}


bool thresh::apply() {
  lti::thresholding::parameters thPar;
  static const std::string filethresh("thresholding.lsp");

  lti::lispStreamHandler lsh;
  std::ifstream in(filethresh.c_str());
  lsh.use(in);
  if (!thPar.read(lsh)) {

    std::ofstream out(filethresh.c_str());
    lsh.use(out);
    thPar.write(lsh);
    out << std::endl;
    out.close();
  }
  in.close();

  help();

  lti::thresholding thresh;

  lti::ioImage loader;
  lti::image img;
  lti::channel chnl;
  lti::channel8 chnl8;
  lti::channel8 mask;
  lti::imatrix imask;
  lti::ipoint pos;

  if (!loader.load(imgFile_,img)) {
    std::cerr << "Image '" << imgFile_ << "' could not be read: "
              << loader.getStatusString() << std::endl;
    exit(EXIT_FAILURE);
  }

  chnl.castFrom(img);
  chnl8.castFrom(img);

  static lti::viewer2D oview("input image");
  oview.show(chnl8);

  // compute the histogram for display
  lti::vector<int> histo(256,0);
  lti::channel8::const_iterator it(chnl8.begin()),eit(chnl8.end());
  
  while (it!=eit) {
    histo.at(*it)++;
    ++it;
  }
  
  static lti::viewer1D hview("Histogram");
  hview.show(histo);


  lti::viewer2D::interaction action;
  lti::viewer2D view("thresholding");

  bool high = false; // flag to indicate if we change the low or the high thr.

  do {
    thresh.setParameters(thPar);

    switch(task_) {
    case Byte:
      thresh.apply(chnl8,mask);
      break;
    case Float:
      thresh.apply(chnl,mask);
      break;
    default:
      thresh.apply(img,imask);
      mask.castFrom(imask);
      break;
    }      

    view.show(mask);
    view.waitInteraction(action,pos);

    switch (action.action) {
    case lti::viewer2D::KeyPressed:
      switch(action.key) {
      case 'h': 
        high=true;
        std::cout << "Setting high threshold" << std::endl;
        break;
      case 'l':
        high=false;
        std::cout << "Setting low threshold" << std::endl;
        break;
      case 'o':
        thPar.method = lti::thresholding::Otsu;
        std::cout << "Otsu method" << std::endl;
        break;
      case 'O':
        thPar.method = lti::thresholding::OtsuInterval;
        std::cout << "Otsu method in interval" << std::endl;
        break;
      case 's':
        thPar.method = lti::thresholding::Simple;
        std::cout << "Simple method" << std::endl;
        break;
      case 'S':
        thPar.method = lti::thresholding::SimpleInterval;
        std::cout << "Simple method in interval" << std::endl;
        break;
      case 'd':
        thPar.method = lti::thresholding::Direct;
        std::cout << "Direct method" << std::endl;
        break;
      case 'r':
        thPar.method = lti::thresholding::Relative;
        std::cout << "Relative method" << std::endl;
        break;
      case 'b':
        thPar.keepBackground = !thPar.keepBackground;

        if (thPar.keepBackground) {
          std::cout << "Keep background" << std::endl;
        } else {
          std::cout << "Replace background" << std::endl;
        }
        break;
      case 'f':
        thPar.keepForeground = !thPar.keepForeground;
        if (thPar.keepForeground) {
          std::cout << "Keep foreground" << std::endl;
        } else {
          std::cout << "Replace foreground" << std::endl;
        }
        break;
      case '?':
        help();
        break;
      case 65362: // up
      case 65363: // right
        if (high) {
          thPar.foreground.to += 0.01;
          if (thPar.foreground.to >= 1.0f) {
            thPar.foreground.to = 1.0f;
          }
        } else {
          thPar.foreground.from += 0.01;
          if (thPar.foreground.from >= thPar.foreground.to) {
            thPar.foreground.from = thPar.foreground.to;
          }
        }
        std::cout << "  Foreground in [" 
                  << thPar.foreground.from
                  << "," 
                  << thPar.foreground.to << "]"
                  << std::endl;
        
        break;
      case 65361: // left
      case 65364: // down

        if (high) {
          thPar.foreground.to -= 0.01;
          if (thPar.foreground.to <= thPar.foreground.from) {
            thPar.foreground.to = thPar.foreground.from;
          }
        } else {
          thPar.foreground.from -= 0.01;
          if (thPar.foreground.from < 0.0f) {
            thPar.foreground.from = 0.0f;
          }
        }
        std::cout << "  Foreground in [" 
                  << thPar.foreground.from
                  << "," 
                  << thPar.foreground.to << "]"
                  << std::endl;
        
        break;

      default:
        std::cout << "Key " << action.key << " unassigned" << std::endl;
        break;
      }
      break;
    default:
      break;
    }

    

  } while (action.action != lti::viewer2D::Closed);

  return false;
}

/*
 * Main method
 */
int main(int argc, char* argv[]) {
  
  thresh me(argc,argv);
  if (me.apply()) {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
    

}
