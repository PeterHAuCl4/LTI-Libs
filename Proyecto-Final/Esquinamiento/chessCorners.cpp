// LTI-Lib Headers
#include "ltiObject.h"
#include "ltiMath.h"     // General lti:: math and <cmath> functionality
#include "ltiConfig.h"   // To check if GTK is there
#include "ltiIOImage.h"  // To read images
#include "ltiChannel8.h" // Monochromatic byte channels

#include "ltiChessCornerness.h"
#include "ltiLispStreamHandler.h"
#include "ltiDraw.h"
#include "ltiLocalExtremes.h"


#if HAVE_GTK
#include "ltiViewer2D.h" // The normal viewer
typedef lti::viewer2D viewer_type;
#else
#include "ltiExternViewer2D.h"  // An emergency exit to display images.
typedef lti::externViewer2D viewer_type;
#endif

#include "ltiV4l2.h"

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
  cout << "Usage: chessCorners [options]  image [-h]" << endl;
  cout << "Find chess features in the given image or in from the given ";
  cout << "camera device.\n";
  cout << "The file \"chess.dat\" allows to configure the ";
  cout << "behaviour of the detector.\n" << endl;
  cout << "  -h show this help." << endl;
  cout << "  -c use camera input instead of file (give the device file if \n"
       << "     needed, such as /dev/video1" << endl;
}

/*
 * Parse the line command arguments
 */
void parseArgs(int argc, char*argv[], 
               std::string& filename,
	       bool& camera) {
  
  camera=false;
  filename.clear();
  // check each argument of the command line
  for (int i=1; i<argc; i++) {
    if (*argv[i] == '-') {
      switch (argv[i][1]) {
      case 'h':
	usage();
	exit(EXIT_SUCCESS);
	break;
      case 'c':
	camera=true;
	break;
      case '-':
	if (std::string(argv[i]) == "--help") {
	  usage();
	  exit(EXIT_SUCCESS);
	}
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

  std::string file;
  bool camera;
  parseArgs(argc,argv,file,camera);

  static const char* confFile = "chess.dat";

  lti::chessCornerness::parameters ccPar;
  lti::localExtremes::parameters lePar;

  // try to read the configuration file
  std::ifstream in(confFile);
  bool write=true;
  if (in) {
    lti::lispStreamHandler lsh;
    lsh.use(in);
    write=!(ccPar.read(lsh) && lePar.read(lsh));
  }
  if (write) {
    // something went wrong reading, write a new configuration file
    std::ofstream out(confFile);
    lti::lispStreamHandler lsh;
    lsh.use(out);
    ccPar.write(lsh);
    lePar.write(lsh);
    out << std::endl;
    out.close();

  }
  
  // create the detector with the user specified configuration
  lti::chessCornerness detector(ccPar);
  lti::localExtremes ext(lePar);
  lePar.relativeThreshold = 0.5;

  if (!camera && file.empty()) {
    usage();
    return EXIT_SUCCESS;
  }

  lti::channel8 chnl;
  lti::ipointList corners;
  
  lti::image canvas;
  lti::draw<lti::rgbaPixel> painter;
  painter.use(canvas);
  
  lti::viewer2D viewo("Original"),viewc("Chess Corners"),viewm("Max Pts");
  
  lti::ipointList::const_iterator it;
  lti::channel cornerness;
  


  if (camera) {
#ifndef _USE_V4L2
    std::cout << "No camera support found" << std::endl;
    return EXIT_FAILURE;
#else
    static const char* camFile = "v4l2.dat";
    std::ifstream in(camFile);
    lti::v4l2::parameters vparam;

    bool write=true;
    if (in) {
      lti::lispStreamHandler lsh;
      lsh.use(in);
      write=!vparam.read(lsh);
      in.close();
    }

    // check if user specified a device
    if (!file.empty()) {
      vparam.deviceFile=file;
    }

    if (write) {
      // something went wrong reading, write a new configuration file
      std::ofstream out(camFile);
      lti::lispStreamHandler lsh;
      lsh.use(out);
      vparam.write(lsh);
      out.close();
    }    

    // camera
    lti::v4l2 cam(vparam);
    
    do {
      if (cam.apply(chnl)) {
        
	detector.apply(chnl,cornerness);
        ext.apply(cornerness,corners);

	// paint the corners
	canvas.castFrom(chnl);
    
	for (it=corners.begin();it!=corners.end();++it) {
	  painter.setColor(lti::rgbaPixel(255,192,100));
	  painter.marker(*it,"x");
	}
	
	viewo.show(chnl);
        viewc.show(cornerness);
	viewm.show(canvas);
      } else {
	std::cerr << "Camera error: " << cam.getStatusString() << std::endl;
	camera=false;
      }
    } while(camera);
#endif  
  } else {
    lti::ioImage loader;
    lti::image img;

    if (!loader.load(file,img)) {
      std::cout << loader.getStatusString() << std::endl;
      return EXIT_FAILURE;
    }
    chnl.castFrom(img);

    detector.apply(chnl,cornerness);
    ext.apply(cornerness,corners);
    
    // paint the corners
    canvas.castFrom(chnl);
    
    for (it=corners.begin();it!=corners.end();++it) {
      painter.setColor(lti::rgbaPixel(255,192,100));
      painter.marker(*it,"x");
    }
    
    viewo.show(chnl);
    viewc.show(cornerness);
    viewm.show(canvas);
    
    std::cout << "Press Enter to continue" << std::endl;
    getchar();
  }

  return EXIT_SUCCESS;
}
