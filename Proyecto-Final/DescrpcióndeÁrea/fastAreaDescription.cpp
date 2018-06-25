
#include <cstdlib>
#include <string>
#include <list>
#include <fstream>
#include <iostream>

#include <ltiFastAreaDescription.h>
#include <ltiIOImage.h>
#include <ltiViewer2D.h>
#include <ltiLispStreamHandler.h>
#include <ltiKMColorQuantization.h>
#include <ltiDraw.h>

/**
 * Just a container for the example
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
   * Number of colors in the quantification
   */
  int numColors_;

  /**
   * Labeling functor
   */
  lti::fastAreaDescription labeler_;

  /**
   * Print the usage of this example.
   */
  void usage(int argc,char *argv[]);

  /**
   * Parse the commands
   */
  bool parse(int argc,char *argv[],std::list<std::string>& files);
};

const char *const example::defaultConfigFile_ = "fastAreaDescription.cfg";

std::ostream& operator<<(std::ostream& s,const lti::ioObject& o) {
  lti::lispStreamHandler lsh(s);
  o.write(lsh);
  return s;
}


example::example() {
  // initialization of the attributes
  configurationFile_  = defaultConfigFile_;
  numColors_ = 4;
}

bool example::read(lti::ioHandler& handler) {
  bool b = true;

  b = lti::read(handler,"numColors",numColors_) && b;
  lti::fastAreaDescription::parameters fad;
  b = lti::read(handler,"fastAreaDescription",fad);
  labeler_.setParameters(fad);
  return b;
}

bool example::write(lti::ioHandler& handler) const {
  bool b = true;

  b = lti::write(handler,"numColors",numColors_) && b;
  b = lti::write(handler,"fastAreaDescription",labeler_.getParameters());

  return b;
}

void example::usage(int argc,char *argv[]) {
  std::cout << "\nUsage: " << argv[0] << " [image] \n\n"; 
  std::cout << "  -c num colors\n";
  std::cout << "  -h Show this help\n" << std::endl;
}

bool example::parse(int argc,char *argv[],std::list<std::string>& files) {
  int i=1;

  files.clear();

  while (i<argc) {
    if ( (std::string(argv[i]) == "-h") ||
         (std::string(argv[i]) == "--help") ) {
      usage(argc,argv);
      return false;
    } else if ( (std::string(argv[i]) == "-c") ) {
      ++i;
      if (i<argc) {
        numColors_ = atoi(argv[i]);
      }
    } else {
      files.push_back(argv[i]);
    }
    ++i;
  }

  return true;
}

int example::operator()(int argc,char *argv[]) {
  std::list<std::string> argFiles;

  //
  // read the configuration file, and write it if something went wrong
  //
  std::ifstream in(configurationFile_.c_str());
  bool writeConfig = true;
  if (in) {
    lti::lispStreamHandler lsh(in);
    writeConfig = !read(lsh);
  }

  if (!parse(argc,argv,argFiles)) {
    return EXIT_FAILURE;
  }

  if (writeConfig) {
    std::ofstream out(configurationFile_.c_str());
    lti::lispStreamHandler lsh(out);
    if (!write(lsh)) {
      std::cerr << "Could not write file '" << configurationFile_ << "'" 
                << std::endl;

      return EXIT_FAILURE;
    }
    out << std::endl;
    out.close();
  }

  in.close();

  //
  // Now the real job!
  //

  lti::image img;

  // create or get an image

  if (argFiles.empty()) {
    // create an image
    img.assign(512,512,lti::rgbaPixel(0,0,0,0));
    for (int y=0;y<img.rows();++y) {
      for (int x=0;x<img.columns();++x) {
        img.at(x,y)=lti::rgbaPixel(x%256,abs(y-x)%256,(x+y)%256);
      }
    }
  } else {
    std::list<std::string>::const_iterator it = argFiles.begin();
    bool ok=true;
    lti::ioImage loader;
    // read the first valid image
    do {
      ok = loader.load(*it,img);
    } while (it!=argFiles.end() && !ok);
  }

  lti::kMColorQuantization::parameters qPar;
  qPar.numberOfColors = numColors_;

  lti::kMColorQuantization quant(qPar);
  
  // convert to image to labeled mask
  lti::channel8 imask;

  // Produce some labels
  lti::palette pal;
  quant.apply(img,imask,pal);

  lti::imatrix mask;
  std::vector< lti::areaDescriptor > desc;
  
  // get the new labels and descriptors
  labeler_.apply(imask,mask,desc);

  // prepare the viewer
  lti::viewer2D::parameters vpar;
  vpar.title = "Labels";
  vpar.labelAdjacency = true;
  vpar.minAdjacencyColors = false;

  lti::viewer2D view(vpar);
  lti::ipoint pos;
  lti::viewer2D::interaction action;
  bool bye=false;

  lti::imatrix canvas(mask);
  lti::draw<int> painter;
  painter.use(canvas);
  painter.setColor(static_cast<int>(desc.size()));

  std::cout << "Click on a region in the viewer to get its data" << std::endl;

  // wait for the user to close the viewer or to select one region to
  // check its descritor values
  do {
    view.show(canvas);
    if (view.waitButtonPressed(action,pos)) {
      
      int label = mask.at(pos);
      int idx = label;
      if ((idx >=0) && (idx < static_cast<int>(desc.size()))) {
        const lti::areaDescriptor& d = desc.at(idx);
        std::cout << "Region label " << label << ":\n" << d << std::endl;
        canvas.copy(mask);

        painter.rectangle(d.computeBoundingBox());
        painter.marker(d.minX,"o");
        painter.marker(d.maxX,"o");
        painter.marker(d.minY,"o");
        painter.marker(d.maxY,"o");
        painter.marker(lti::iround(d.cog.x),lti::iround(d.cog.y),"+");
      }

      // show information of the selected mask
    } else {
      bye = true;
    }
  } while( !bye);


  return EXIT_SUCCESS;
}


int main(int argc,char *argv[]) {
  try {
    example obj;
    return obj(argc,argv);
  }
  catch (lti::exception& exp) {
    std::cout << "An LTI::EXCEPTION was thrown: ";
    std::cout << exp.what() << std::endl;
  }
  catch (std::exception& exp) {
    std::cout << "std::exception was thrown: ";
    std::cout << exp.what() << std::endl;
  }
  catch (...) {
    std::cout << "Unknown exception thrown!" << std::endl;
  }
  return EXIT_FAILURE;

} // main
