/*
 * Copyright (C) 2008-2012
 * Pablo Alvarado
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
 * \file   colorProbability.h
 *         Contains an example for the use of the class
 *         lti::colorProbabilityMap.
 * \author Pablo Alvarado
 * \date   27.12.2008
 *
 * revisions ..: $Id: chessCorners.h,v 1.1 2013-06-14 01:34:56 alvarado Exp $
 */

#ifndef COLOR_PROBABILITY
#define COLOR_PROBABILITY

#define _WITH_V4L2


#include <string>
#include <list>
#include <ltiLispStreamHandler.h>
#include <ltiColorModelEstimation.h>
#include <ltiColorProbabilityMap.h>
#include <ltiHistogram.h>

#ifdef _WITH_V4L2
#include "ltiV4l2.h"
#endif

/**
 * Abstraction class to wrap whether a camera data source, or a list of files.
 */
class source {
public:
  /**
   * Which input should be used
   */
  enum eInput {
    Files,
    V4L2
  };

  /**
   * Constructor
   */
  source(const eInput src,
         const std::list<std::string>& files = std::list<std::string>() );

  /**
   * Destructor
   * Saves camera parameters in case an external application like guvcview 
   * changed them
   */
  ~source();

  /**
   * Dump parameters of V4L2 (if available)
   */
  void dump();

  /**
   * Capture the next image
   *
   * @return \c true if a next image could be read or \c false if an error or
   */
  bool next(lti::image& img);

private:
  /**
   * Get next image from file
   */
  bool nextImage(lti::image& img);

  /**
   * Get next image from camera
   */
  bool nextV4l2(lti::image& img);
  
#ifdef _WITH_V4L2
  /**
   * Camera reading class
   */
  lti::v4l2* v4l2_;
#endif

  /**
   * List of files to be read
   */
  std::list<std::string> files_;

  /**
   * Points to the file in the files_ list.
   */
  std::list<std::string>::const_iterator fileIterator_;

  /**
   * Type of image input
   */
  eInput input_;
};

/**
 * Just a container for the example.
 */
class example {
public:
  /**
   * Default constructor
   */
  example();

  /**
   * The execution method
   */
  int operator()(int argc,char *argv[]);

  /**
   * Read example configuration
   */
  bool read(lti::ioHandler& handler);

  /**
   * Write example configuration
   */
  bool write(lti::ioHandler& handler) const;
  
  /**
   * Train
   */
  bool train();

  /**
   * Classify
   */
  bool classify(const std::list<std::string>& imgFiles);

protected:
  /**
   * Default configuration file name
   */
  static const char *const defaultConfigFile_;

  /**
   * Configuration file name
   */
  std::string configurationFile_;

  /**
   * Name of file containing the object model.
   *
   * In training mode, the resulting models will be left in this file.  In
   * classification mode, the model is read from this file.
   */
  std::string objectModelFile_;

  /**
   * Name of file containing the non-object model
   *
   * In training mode, the resulting models will be left in this file.  In
   * classification mode, the model is read from this file.
   */
  std::string nonObjectModelFile_;

  /**
   * Train images file.
   *
   * This attribute contains the name of a file containing a list of image
   * files that should be used for training.
   */
  std::string trainImages_;

  /**
   * Mask suffix
   *
   * For each image in the trainImages_ file, a corresponding mask is searched
   * which will have this suffix appended.
   */
  std::string maskSuffix_;

  /**
   * Parameters
   * 
   * Parameters of the color probability map
   */
  lti::colorProbabilityMap::parameters parameters_;

  /**
   * Get the name of the mask file, given the image file
   */
  std::string getMaskName(const std::string& imgName) const;

private:
  /**
   * Model for the object
   */
  lti::dhistogram objectModel_;

  /**
   * Model for the non-object
   */
  lti::dhistogram nonObjectModel_;

  /**
   * Parameters for the model estimation
   */
  lti::colorModelEstimation::parameters cmePar_;

  /**
   * Train mode
   */
  bool trainMode_;

  /**
   * Source of evaluated images
   */
  source::eInput sourceType_;

  /**
   * Print the usage of this example.
   */
  void usage(int argc,char *argv[]);

  /**
   * Parse the commands
   */
  bool parse(int argc,char *argv[],std::list<std::string>& files);

  /**
   * Wait the user to click or press a key on the viewer 
   */
  bool pause_;

};


#endif
