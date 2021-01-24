#include "ofApp.h"

/*
    If you are struggling to get the device to connect ( especially Windows Users )
    please look at the ReadMe: in addons/ofxKinect/README.md
*/

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
    //ofSetVerticalSync(false);

	// enable depth->video image calibration
    kinect.setRegistration(true);
    
    kinect.init();
    //kinect.init(true); // shows infrared instead of RGB video image
    //kinect.init(false, false); // disable video image (faster fps)
	
	kinect.open();		// opens first available kinect
	//kinect.open(1);	// open a kinect by id, starting with 0 (sorted by serial # lexicographically))
	//kinect.open("A00362A08602047A");	// open a kinect using it's unique serial #
	
	// print the intrinsic IR sensor values
    if(kinect.isConnected()) {
    //	ofLogNotice() << "sensor-emitter dist: " << kinect.getSensorEmitterDistance() << "cm";
    //	ofLogNotice() << "sensor-camera dist:  " << kinect.getSensorCameraDistance() << "cm";
    //	ofLogNotice() << "zero plane pixel size: " << kinect.getZeroPlanePixelSize() << "mm";
    //	ofLogNotice() << "zero plane dist: " << kinect.getZeroPlaneDistance() << "mm";
	}
	
    colorImg.allocate(kinect.depthWidth, kinect.depthHeight);
    grayImage.allocate(kinect.depthWidth, kinect.depthHeight);
    grayThreshNear.allocate(kinect.depthWidth, kinect.depthHeight);
    grayThreshFar.allocate(kinect.depthWidth, kinect.depthHeight);
	
	nearThreshold = 230;
	farThreshold = 70;
	bThreshWithOpenCV = true;	
	
	// start from the front
    bDrawPointCloud = false;
}

//--------------------------------------------------------------
void ofApp::update() {
    ofSetWindowTitle("Fps: "+ofToString(ofGetFrameRate()));
	ofBackground(100, 100, 100);
	
	kinect.update();
	
	// there is a new frame and we are connected
	if(kinect.isFrameNew()) {        

        // load grayscale depth image from the kinect source
        grayImage.setFromPixels(kinect.getDepthPixels());
		
        // we do two thresholds - one for the far plane and one for the near plane
        // we then do a cvAnd to get the pixels which are a union of the two thresholds
        if(bThreshWithOpenCV) {
            grayThreshNear = grayImage;
            grayThreshFar = grayImage;
            grayThreshNear.threshold(nearThreshold, true);
            grayThreshFar.threshold(farThreshold);
            cvAnd(grayThreshNear.getCvImage(), grayThreshFar.getCvImage(), grayImage.getCvImage(), NULL);
        } else {
			
            // or we do it ourselves - show people how they can work with the pixels
            ofPixels & pix = grayImage.getPixels();
            int numPixels = pix.size();
            for(int i = 0; i < numPixels; i++) {
                if(pix[i] < nearThreshold && pix[i] > farThreshold) {
                    pix[i] = 255;
                } else {
                    pix[i] = 0;
                }
            }
        }
		
        // update the cv images
        grayImage.flagImageChanged();
		
        // find contours which are between the size of 20 pixels and 1/3 the w*h pixels.
        // also, find holes is set to true so we will get interior contours as well....
        contourFinder.findContours(grayImage, 100, (kinect.depthWidth*kinect.depthHeight)/2, 20, false);
	}
	
}

//--------------------------------------------------------------
void ofApp::draw() {

	ofSetColor(255, 255, 255);
	
    if(bDrawPointCloud) {
        easyCam.begin();
		drawPointCloud();
        easyCam.end();
	} else {
		// draw from the live kinect
        float ratio = (float)kinect.height/(float)kinect.width;
        int imgW = 450;

        kinect.drawDepth(10, 10, imgW, ratio*imgW);
        kinect.draw(imgW+20, 10, imgW, ratio*imgW);
		
        grayImage.draw(10, ratio*imgW+20, imgW, ratio*imgW);
        contourFinder.draw(10, ratio*imgW+20, imgW, ratio*imgW);
		
	}
	
	// draw instructions
	ofSetColor(255, 255, 255);
	stringstream reportStream;
        
    
	reportStream << "press p to switch between images and point cloud, rotate the point cloud with the mouse" << endl
	<< "using opencv threshold = " << bThreshWithOpenCV <<" (press spacebar)" << endl
	<< "set near threshold " << nearThreshold << " (press: + -)" << endl
    << "set far threshold " << farThreshold << " (press: < >) num blobs found " << contourFinder.nBlobs << endl
    << "connection is: " << kinect.isConnected() << endl;

    ofDrawBitmapString(reportStream.str(), 20, 700);
}

void ofApp::drawPointCloud() {
    int w = kinect.getDepthPixels().getWidth();
    int h = kinect.getDepthPixels().getHeight();
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_POINTS);
    int step = 2;
    for(int y = 0; y < h; y += step) {
        for(int x = 0; x < w; x += step) {
            //if(kinect.getDistanceAt(x, y) > 0)
            {
                if(kinect.getWorldCoordinateAt(x, y).z < 1.5f)
                {
                    mesh.addColor(kinect.getColorAt(x,y));
                    //mesh.addColor(ofColor(255));
                    mesh.addVertex(kinect.getWorldCoordinateAt(x, y));
                }
            }
        }
    }
    glPointSize(2);
	ofPushMatrix();
	// the projected points are 'upside down' and 'backwards' 
    ofScale(2000, -2000, -2000);
    mesh.draw();
	ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::exit() {

	kinect.close();

}

//--------------------------------------------------------------
void ofApp::keyPressed (int key) {
	switch (key) {
		case ' ':
			bThreshWithOpenCV = !bThreshWithOpenCV;
			break;
			
		case'p':
			bDrawPointCloud = !bDrawPointCloud;
			break;
			
		case '>':
		case '.':
			farThreshold ++;
			if (farThreshold > 255) farThreshold = 255;
			break;
			
		case '<':
		case ',':
			farThreshold --;
			if (farThreshold < 0) farThreshold = 0;
			break;
			
		case '+':
		case '=':
			nearThreshold ++;
			if (nearThreshold > 255) nearThreshold = 255;
			break;
			
		case '-':
			nearThreshold --;
			if (nearThreshold < 0) nearThreshold = 0;
			break;
			
		case 'w':
            //kinect.enableDepthNearValueWhite(!kinect.isDepthNearValueWhite());
			break;


	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
	
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{

}
