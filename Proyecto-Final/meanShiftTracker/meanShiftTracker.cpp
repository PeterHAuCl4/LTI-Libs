/*
 * Copyright (C) 2007 by Pablo Alvarado
 * 
 * This file is part of the LTI-Computer Vision Library 2 (LTI-Lib-2)
 *
 * The LTI-Lib-2 is free software; you can redistribute it and/or
 * modify it under the terms of the BSD License.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the authors nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/** 
 * \file   surfLocalDescriptor.cpp
 *         Contains an example of use for the class lti::surfLocalDescriptor
 * \author Pablo Alvarado
 * \date   04.11.2007
 * revisions ..: $Id: meanShiftTracker.cpp,v 1.3 2010-02-09 15:58:08 alvarado Exp $
 */

// LTI-Lib Headers
#include "ltiObject.h"
#include "ltiIOImage.h"
#include "ltiMath.h"
#include "ltiPassiveWait.h"

#include "ltiMatrixTransform.h"
#include "ltiBilinearInterpolation.h"
#include "ltiMeanShiftTracker.h"
#include "ltiDraw.h"

#include "ltiLispStreamHandler.h"

#include "ltiViewer2D.h" // The normal viewer
typedef lti::viewer2D viewer_type;

// Standard Headers
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

using std::cout;
using std::cerr;
using std::endl;

/*
 * Help 
 */
void usage() {
  cout << "Usage: meanShiftTracker [image] [-h]" << endl;
  cout << "Track a spot with the mean-shift tracker on the given image\n";
  cout << "  -h show this help." << endl;
}

/*
 * Parse the line command arguments
 */
void parseArgs(int argc, char*argv[], 
               std::string& filename) {
  
  filename.clear();
  // check each argument of the command line
  for (int i=1; i<argc; i++) {
    if (*argv[i] == '-') {
      switch (argv[i][1]) {
        case 'h':
          usage();
          exit(EXIT_SUCCESS);
          break;
        default:
          break;
      }
    } else {
      filename = argv[i]; // guess that this is the filename
    }
  }
}

/*
 * Main method
 */
int main(int argc, char* argv[]) {

  std::string imgFile;
  parseArgs(argc,argv,imgFile);

  if (imgFile.empty()) {
    usage();
    exit(EXIT_FAILURE);
  }

  static const char* confFile = "meanShiftTracker.dat";

  lti::ioImage loader; // used to load an image file

  lti::image img;
  if (!loader.load(imgFile,img)) {
    std::cerr << "Could not read " << imgFile << ": "
              << loader.getStatusString()
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // matrixTransform is used to rotate the image
  typedef lti::matrixTransform< lti::rgbaPixel> trans_type;
  
  trans_type::parameters transPar;
  transPar.resizeMode = lti::geometricTransformBase::KeepDimensions;
  transPar.interpolatorParams.boundaryType = lti::Periodic;

  lti::meanShiftTracker::parameters mstPar;

  std::ifstream in(confFile);
  bool write=true;

  float maxPan = 45;
  float angleStep = 5;
  int numTurns = 10;

  if (in) {
    lti::lispStreamHandler lsh;
    lsh.use(in);
    write=!transPar.read(lsh);
    write=write || !lti::read(lsh,"meanShiftTracker",mstPar);
    write=write || !lti::read(lsh,"maxPan",maxPan);
    write=write || !lti::read(lsh,"angleStep",angleStep);
    write=write || !lti::read(lsh,"numTurns",numTurns);
  }
  if (write) {
    // something went wrong loading the data, so just write again to fix
    // the errors
    std::ofstream out(confFile);
    lti::lispStreamHandler lsh;
    lsh.use(out);
    transPar.write(lsh);
    lti::write(lsh,"meanShiftTracker",mstPar);
    lti::write(lsh,"maxPan",maxPan);
    lti::write(lsh,"angleStep",angleStep);
    lti::write(lsh,"numTurns",numTurns);
    out<<std::endl;
  }

  lti::meanShiftTracker mst(mstPar);
  trans_type transformer(transPar);
  lti::draw<lti::rgbaPixel> painter;

  lti::image res;
  painter.use(res);
  painter.setColor(lti::rgbaPixel(255,128,128));

  lti::viewer2D view("Transformed");
  lti::viewer2D::interaction action;
  lti::ipoint pos;
  lti::fmatrix proj(4,4,0.0f);
  float pan;
  float angle = 0.0;

  proj.setIdentity(1.0f);
  proj.at(3,2) = 1.0f/500.0f;
  proj.at(0,2) = proj.at(3,2)*img.columns()/2;
  proj.at(1,2) = proj.at(3,2)*img.rows()/2;

  bool tracking = false;
  lti::irectangle window;
  window.resize(33,33);

  std::cout << "\nClick on the window to track the area around the " \
    "indicated position\n" << std::endl;

  do {
    lti::fmatrix rot;
    pan = maxPan*(1.0f - lti::cos(lti::degToRad(angle/numTurns)))/2;
    float rangle = lti::degToRad(angle);
    rot=lti::rotationMatrix(lti::fpoint3D(img.columns()/2,img.rows()/2,0),
                            lti::fpoint3D(cos(rangle)*100,
                                          sin(rangle)*100,
                                          cos(rangle)*sin(rangle)*150),
                            lti::degToRad(pan));

    transformer.setMatrix(proj*rot);
    transformer.apply(img,res);
  
    angle  += angleStep;
    if (angle>=numTurns*360) {
      angle = 0;
    }

    std::cout << "Rotation at " << static_cast<int>(angle)%360 
              << " of " << static_cast<int>(pan) << "            \r";
    std::cout.flush();

    if (tracking) {
      mst.apply(res,window);
      painter.rectangle(window);
    }

    view.show(res);
    view.getLastAction(action,pos);

    if ( (action.action == lti::viewer2D::ButtonPressed) and
         (action.key    == lti::viewer2D::LeftButton) ) {
      window.resize(33,33);
      window.setCenter(pos);
      mst.initialize(res,window);
      tracking=true;
    }
  } while(action.action != lti::viewer2D::Closed);
  

  return EXIT_SUCCESS;
}
