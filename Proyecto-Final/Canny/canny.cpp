
#include "canny.h"

// LTI-Lib Headers

#include <ltiObject.h>
#include <ltiMath.h>     // General lti:: math and <cmath> functionality
#include <ltiTimer.h>    // To measure time

#include <ltiLispStreamHandler.h>

#include <ltiCannyEdges.h>

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


canny::canny(int argc, char* argv[]) 
  : task_(None) {
  parse(argc,argv);
}


/*
 * Help 
 */
void canny::usage() const {
  cout <<
    "usage: canny [options] <image> \n\n" \
    "       -f      Use gray-channel of floats\n" \
    "       -8      Use gray-channel of bytes\n" \
    "       <image>  input image" << std::endl; 
}

void canny::help() const {
  cout << 

    " v      Select kernel variance.\n" \
    " k      Select kernel size.\n" \
    " m      Select max theshold (all > IS edge).\n" \
    " l      Select low theshold (edge only if neighbor is edge).\n" \
    " g      Gradient type.\n" \
    " Arrows Increase/Decrease selected value.\n" \
    " ?      Print this message.\n" << std::endl;
}

/*
 * Parse the line command arguments
 */
void canny::parse(int argc, char*argv[]) {

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


bool canny::apply() {
  lti::cannyEdges::parameters thPar;
  static const char* filecanny = "canny.lsp";

  lti::lispStreamHandler lsh;
  std::ifstream in(filecanny);
  lsh.use(in);
  if (!thPar.read(lsh)) {

    std::ofstream out(filecanny);
    lsh.use(out);
    thPar.write(lsh);
    out << std::endl;
    out.close();
  }
  in.close();

  help();

  lti::cannyEdges canny(thPar);

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

  lti::viewer2D::interaction action;
  lti::viewer2D view("canny");

  const lti::gradientFunctor::eKernelType ktypes[] = {
    lti::gradientFunctor::Ando,
    lti::gradientFunctor::OGD,
    lti::gradientFunctor::Difference,
    lti::gradientFunctor::Roberts,
    lti::gradientFunctor::Sobel,
    lti::gradientFunctor::Prewitt,
    lti::gradientFunctor::Robinson,
    lti::gradientFunctor::Kirsch,
    lti::gradientFunctor::Harris
  };

  const char* ktypesNames[] = {
    "Ando",
    "OGD",
    "Difference",
    "Roberts",
    "Sobel",
    "Prewitt",
    "Robinson",
    "Kirsch",
    "Harris"
  };

  enum eState {
    KVariance,
    KSize,
    MxThresh,
    MnThresh,
    GType
  };

  eState state=GType;

  int ktypeIdx = 0;

  do {
    canny.setParameters(thPar);

    switch(task_) {
    case Byte:
      canny.apply(chnl8,mask);
      break;
    case Float:
      canny.apply(chnl,mask);
      break;
    default:
      canny.apply(img,mask);
      break;
    }      

    view.show(mask);
    view.waitInteraction(action,pos);

    switch (action.action) {
    case lti::viewer2D::KeyPressed:
      switch(action.key) {
      case 'v': 
	std::cout << "Setting gaussian kernel variance" << std::endl;
	state = KVariance;
        break;
      case 'k': 
	std::cout << "Setting gaussian kernel size" << std::endl;
	state = KSize;
        break;
      case 'm':
        std::cout << "Setting max threshold (if > then is edge)" << std::endl;
	state = MxThresh;
        break;
      case 'l':
        std::cout << "Setting min threshold (if neighbor and > then is edge)" 
		  << std::endl;
	state = MnThresh;
        break;
      case 'g':
	std::cout << "Setting gradient kernel type" << std::endl;
	state = GType;
      case '?':
        help();
        break;
      case lti::viewer2D::UpKey: // up
      case lti::viewer2D::RightKey: { // right 
	
	float step;
	if (action.key == lti::viewer2D::UpKey) {
	  step=0.1f;
	} else {
	  step=0.01f;
	}

	switch(state) {
	case KVariance:
	  thPar.variance+=step;
	  std::cout << "New kernel variance: " << thPar.variance << std::endl;
	  break;
	case KSize:
	  thPar.kernelSize++;
	  std::cout << "New kernel size: " << thPar.kernelSize << std::endl;
	  break;
	case MxThresh:
	  thPar.thresholdMax = lti::min(1.0f,thPar.thresholdMax+step);
	  std::cout << "New max threshold: " << thPar.thresholdMax << std::endl;
	  break;
	case MnThresh:
	  thPar.thresholdMin = lti::min(1.0f,thPar.thresholdMin+step);
	  std::cout << "New min threshold: " << thPar.thresholdMin << std::endl;
	  break;
	case GType:
	  ktypeIdx=(ktypeIdx+1)%9;
	  thPar.gradientParameters.kernelType = ktypes[ktypeIdx];
	  std::cout << "New kernel size: "<<ktypesNames[ktypeIdx]<<std::endl;
	  break;
	default:
	  break;
	}
      } break;

      case lti::viewer2D::DownKey:   // down
      case lti::viewer2D::LeftKey: { // left

	float step;
	if (action.key == lti::viewer2D::DownKey) {
	  step=0.1f;
	} else {
	  step=0.01f;
	}

	switch(state) {
	case KVariance:
	  thPar.variance=lti::max(0.0f,thPar.variance-step);
	  std::cout << "New kernel variance: " << thPar.variance << std::endl;
	  break;
	case KSize:
	  thPar.kernelSize=lti::max(1,thPar.kernelSize-1);
	  std::cout << "New kernel size: " << thPar.kernelSize << std::endl;
	  break;
	case MxThresh:
	  thPar.thresholdMax = lti::max(0.0f,thPar.thresholdMax-step);
	  std::cout << "New max threshold: " << thPar.thresholdMax << std::endl;
	  break;
	case MnThresh:
	  thPar.thresholdMin = lti::max(0.0f,thPar.thresholdMin-step);
	  std::cout << "New min threshold: " << thPar.thresholdMin << std::endl;
	  break;
	case GType:
	  ktypeIdx=(ktypeIdx+8)%9;
	  thPar.gradientParameters.kernelType = ktypes[ktypeIdx];
	  std::cout << "New kernel size: "<<ktypesNames[ktypeIdx]<<std::endl;
	  break;
	default:
	  break;
	}
      } break;

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
  
  canny me(argc,argv);
  if (me.apply()) {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
    

}
