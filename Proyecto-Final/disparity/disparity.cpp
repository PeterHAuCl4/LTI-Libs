
#include <ltiObject.h>
#include <ltiMath.h>     // General lti:: math and <cmath> functionality
#include <ltiTimer.h>    // To measure time

#include <ltiLispStreamHandler.h>

#include <ltiDraw.h>
#include <ltiViewer2D.h>
#include <ltiViewer1D.h>
#include <ltiImage.h>
#include <ltiIOImage.h>
#include <ltiColors.h>

#include <ltiMatrixTransform.h>

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


disparity::disparity(int argc, char* argv[]) : range_(20) {
  parse(argc,argv);
}


/*
 * Help 
 */
void disparity::usage() const {
  cout <<
    "usage: disparity [options] <image1> <image2> \n\n" \
    "       -r val  Disparity range\n" \
    "       -l row  Generate an example row disparity\n" \
    "       -h      Show this help\n" \
    "       <image1> left input image\n" \
    "       <image2> right input image" << std::endl; 
}

void disparity::help() const {
  cout << 
    " +      Increase disparity.\n" \
    " -      Decrease disparity.\n" \
    " Arrows Increase/Decrease selected disparity.\n" \
    " ?      Print this message.\n" << std::endl;
}

/*
 * Parse the line command arguments
 */
void disparity::parse(int argc, char*argv[]) {

  int c;

  // We use the standard getopt.h functions here to parse the arguments.
  // Check the documentation of getopt.h for more information on this
  
  // structure for the long options. 
  static struct option lopts[] = {
    {"help",no_argument,0,'h'},
    {"range",required_argument,0,'r'},
    {"line",required_argument,0,'l'},
    {0,0,0,0}
  };

  int optionIdx;
  line_=-1; // indicate that no line analysis is desired

  while ((c = getopt_long(argc, argv, "hr:l:", lopts,&optionIdx)) != -1) {
    switch (c) {
    case 'r':
      range_=atoi(optarg);
      break;
    case 'l':
      line_=atoi(optarg);
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
  
  imgFile1_=imgFile2_="";

  if (optind < argc) {
    imgFile1_ = argv[optind++];
  }

  if (optind < argc) {
    imgFile2_ = argv[optind++];
  }

  if (imgFile1_.empty() || imgFile2_.empty()) {
    cerr << "Two image files needed" << std::endl;
    usage();
    exit(EXIT_FAILURE);
  }

}

void disparity::lineDisparity(const int line,
			      const lti::channel& left,
			      const lti::channel& right,
			      lti::channel& disparity) {

  if ( (line<0) || (line>=right.rows())) {
    std::cerr << "Invalid line number." << std::endl;
    return;
  }

  lti::matrixTransform<float>::parameters mtPar;
  mtPar.interpolatorParams.boundaryType = lti::Constant;
  lti::matrixTransform<float> mt(mtPar);
  lti::channel tmpDisparity,transRight;

  int pxStep=4; // numbers of steps per pixel (should be 2^x)
  float step=1.0f/pxStep;

  disparity.allocate(pxStep*range_*2+1,right.columns());

  // This is _very_ inefficient code, but it is made "just for fun"
  // Some day, a disparity functor will be made...

  for (float i=-range_;i<=range_;i+=step) {
    lti::fmatrix trans = lti::translationMatrix(lti::fpoint(i,0));
    mt.setMatrix(trans);
    mt.apply(right,transRight);
    
    tmpDisparity.subtract(left,transRight);
    tmpDisparity.apply(lti::abs);

    disparity.getRow((i+range_)*pxStep).copy(tmpDisparity.getRow(line));
  }
}

bool disparity::apply() {

  help();

  lti::ioImage loader;
  lti::image img;
  lti::channel left,right,disparity,transRight;
  lti::ipoint pos;

  if (!loader.load(imgFile1_,img)) {
    std::cerr << "Image '" << imgFile1_ << "' could not be read: "
              << loader.getStatusString() << std::endl;
    exit(EXIT_FAILURE);
  }
  left.castFrom(img);

  if (!loader.load(imgFile2_,img)) {
    std::cerr << "Image '" << imgFile1_ << "' could not be read: "
              << loader.getStatusString() << std::endl;
    exit(EXIT_FAILURE);
  }
  right.castFrom(img);

  static lti::viewer2D lview("Left image");
  static lti::viewer2D rview("Right image");
  lview.show(left);
  rview.show(right);

  static lti::viewer2D ldview("Disparity at line");

  if ((line_>=0) && (line_<right.rows())) {
    lti::channel ld;
    lineDisparity(line_,left,right,ld);
    ldview.show(ld);

    // paint the line in the images to know what they are
    img.castFrom(left);
    img.getRow(line_).fill(lti::rgbaPixel(255,128,0));
    lview.show(img);

    img.castFrom(right);
    img.getRow(line_).fill(lti::rgbaPixel(255,128,0));
    rview.show(img);

  } else {
    lview.show(left);
    rview.show(right);
  }


  lti::matrixTransform<float>::parameters mtPar;
  mtPar.interpolatorParams.boundaryType = lti::Constant;
  lti::matrixTransform<float> mt(mtPar);
  

  lti::viewer2D::interaction action;
  lti::viewer2D view("disparity");

  float d = 0;

  do {
    lti::fmatrix trans = lti::translationMatrix(lti::fpoint(d,0));
    mt.setMatrix(trans);
    mt.apply(right,transRight);
    
    disparity.subtract(left,transRight);
    disparity.apply(lti::abs);

    view.show(disparity);
    view.waitInteraction(action,pos);


    switch (action.action) {
    case lti::viewer2D::KeyPressed:
      switch(action.key) {
      case '?':
        help();
        break;
      case '+':
      case lti::viewer2D::UpKey: 
      case lti::viewer2D::RightKey:
	d+=0.25;
        std::cout << "  Displacement: " << d << std::endl;
        break;
      case '-':
      case lti::viewer2D::DownKey: 
      case lti::viewer2D::LeftKey: 
	d-=0.25;
        std::cout << "  Displacement: " << d << std::endl;
        break;
      default:
        std::cout << "Key " << action.key << " unassigned" << std::endl;
        break;
      }
      break;
    case lti::viewer2D::ButtonPressed: {
      if (action.key == lti::viewer2D::LeftButton) {
	line_=pos.y;
	lti::channel ld;
	lineDisparity(line_,left,right,ld);
	ldview.show(ld);
	
	// paint the line in the images to know what they are
	img.castFrom(left);
	img.getRow(line_).fill(lti::rgbaPixel(255,128,0));
	lview.show(img);
	
	img.castFrom(right);
	img.getRow(line_).fill(lti::rgbaPixel(255,128,0));
	rview.show(img);
      }
    } break;
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
  
  disparity me(argc,argv);
  if (me.apply()) {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
    

}
