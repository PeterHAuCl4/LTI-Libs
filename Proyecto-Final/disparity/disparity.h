/*
 * Copyright (C) 2013 by Pablo Alvarado
 * 
 * This file is part of the lecture CE-5201 Digital Image Processing and
 * Analysis, at the Costa Rica Institute of Technology.
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
 * \file   disparity.h
 *         Examples of disparity 
 * \author Pablo Alvarado
 * \date   30.03.2013
 * revisions ..: $Id: disparity.h,v 1.1 2013-03-31 02:25:37 alvarado Exp $
 */

#include <string>
#include <ltiChannel.h>

/**
 * This class shows disparity measures between a stereo pair
 */
class disparity {
  
public:

  /**
   * Constructor with the command line parameters
   */
  disparity(int argc, char*argv[]);

  /**
   * Print how to use the command line
   */
  void usage() const;

  /**
   * Do the real job.
   * \return true if successful, false otherwise
   */
  bool apply();

private:

  /**
   * Parse the command line arguments
   */
  void parse(int argc, char*argv[]);

  /**
   * Print usage help
   */
  void help() const;

  /**
   * @name Image file names
   */
  //@{
  std::string imgFile1_;
  std::string imgFile2_;
  //@}

  /**
   * Disparity range
   */
  int range_;

  /**
   * Line to generate disparity
   */
  int line_;

  /**
   * Line disparity
   */
  void lineDisparity(const int line,
		     const lti::channel& left,
		     const lti::channel& right,
		     lti::channel& disparity);

};
