#include "Controller.h"

#include "WarpBilinear.h"
#include "WarpPerspective.h"
#include "WarpPerspectiveBilinear.h"

#include "ofMain.h"
#include "GLFW/glfw3.h"

namespace ofxWarp
{
	//--------------------------------------------------------------
	Controller::Controller()
		: focusedIndex(-1)
	{
		ofAddListener(ofEvents().mouseMoved, this, &Controller::onMouseMoved);
		ofAddListener(ofEvents().mousePressed, this, &Controller::onMousePressed);
		ofAddListener(ofEvents().mouseDragged, this, &Controller::onMouseDragged);
		ofAddListener(ofEvents().mouseReleased, this, &Controller::onMouseReleased);

		ofAddListener(ofEvents().keyPressed, this, &Controller::onKeyPressed);
		ofAddListener(ofEvents().keyReleased, this, &Controller::onKeyReleased);

		ofAddListener(ofEvents().windowResized, this, &Controller::onWindowResized);
	}
	
	//--------------------------------------------------------------
	Controller::~Controller()
	{
		ofRemoveListener(ofEvents().mouseMoved, this, &Controller::onMouseMoved);
		ofRemoveListener(ofEvents().mousePressed, this, &Controller::onMousePressed);
		ofRemoveListener(ofEvents().mouseDragged, this, &Controller::onMouseDragged);
		ofRemoveListener(ofEvents().mouseReleased, this, &Controller::onMouseReleased);

		ofRemoveListener(ofEvents().keyPressed, this, &Controller::onKeyPressed);
		ofRemoveListener(ofEvents().keyReleased, this, &Controller::onKeyReleased);

		ofRemoveListener(ofEvents().windowResized, this, &Controller::onWindowResized);
		
		this->warps.clear();
	}

	//--------------------------------------------------------------
	bool Controller::saveSettings(const std::string & filePath)
	{
		nlohmann::json json;
		this->serialize(json);

		auto file = ofFile(filePath, ofFile::WriteOnly);
		file << json.dump(4);

		return true;
	}

	//--------------------------------------------------------------
	bool Controller::loadSettings(const std::string & filePath)
	{
		auto file = ofFile(filePath, ofFile::ReadOnly);
		if (!file.exists())
		{
			ofLogWarning("Warp::loadSettings") << "File not found at path " << filePath;
			return false;
		}

		nlohmann::json json;
		file >> json;

		this->deserialize(json);

		return true;
	}

	//--------------------------------------------------------------
	void Controller::serialize(nlohmann::json & json)
	{
		std::vector<nlohmann::json> jsonWarps;
		for (auto warp : this->warps)
		{
			nlohmann::json jsonWarp;
			warp->serialize(jsonWarp);
			jsonWarps.push_back(jsonWarp);
		}
		json["warps"] = jsonWarps;
	}
	
	//--------------------------------------------------------------
	void Controller::deserialize(const nlohmann::json & json)
	{
		this->warps.clear();
		for (auto & jsonWarp : json["warps"])
		{
			std::shared_ptr<WarpBase> warp;

			int typeAsInt = jsonWarp["type"];
			WarpBase::Type type = (WarpBase::Type)typeAsInt;
			switch (type)
			{
			case WarpBase::TYPE_BILINEAR:
				warp = std::make_shared<WarpBilinear>();
				break;

			case WarpBase::TYPE_PERSPECTIVE:
				warp = std::make_shared<WarpPerspective>();
				break;

			case WarpBase::TYPE_PERSPECTIVE_BILINEAR:
				warp = std::make_shared<WarpPerspectiveBilinear>();
				break;

			default:
				ofLogWarning("Warp::loadSettings") << "Unrecognized Warp type " << type;
			}

			if (warp)
			{
				warp->deserialize(jsonWarp);
				this->warps.push_back(warp);
			}
		}
	}

	//--------------------------------------------------------------
	bool Controller::addWarp(std::shared_ptr<WarpBase> warp)
	{
		auto it = std::find(this->warps.begin(), this->warps.end(), warp);
		if (it == this->warps.end())
		{
			this->warps.push_back(warp);
			return true;
		}

		return false;
	}
	
	//--------------------------------------------------------------
	bool Controller::removeWarp(std::shared_ptr<WarpBase> warp)
	{
		auto it = std::find(this->warps.begin(), this->warps.end(), warp);
		if (it != this->warps.end())
		{
			this->warps.erase(it);
			return true;
		}

		return false;
	}

	//--------------------------------------------------------------
	std::vector<std::shared_ptr<WarpBase>> & Controller::getWarps()
	{
		return this->warps;
	}

	//--------------------------------------------------------------
	std::shared_ptr<WarpBase> Controller::getWarp(size_t index) const
	{
		if (index < this->warps.size())
		{
			return this->warps[index];
		}
		return nullptr;
	}
	
	//--------------------------------------------------------------
	size_t Controller::getNumWarps() const
	{
		return this->warps.size();
	}

#pragma mark CONTROL POINTS AND WARPS
    
    //--------------------------------------------------------------
    int Controller::findClosestControlPoint(const glm::vec2 & pos, float maxDist)
    {
        int pointIdx = -1;
        auto distance = std::numeric_limits<float>::max();
        
        // Find warp and distance to closest control point.
        for (int i = this->warps.size() - 1; i >= 0; --i)
        {
			if(i == focusedIndex){
				float candidate;
				auto idx = this->warps[i]->findClosestControlPoint(pos, &candidate);
				if (candidate < distance && this->warps[i]->isEditing())
				{
					distance = candidate;
					pointIdx = idx;
				}
			}
        }
		if(distance < maxDist){
        		return pointIdx;
		}else{
			return -1;
		}
    }

void Controller::draw(){

	if(squareSelect){
		ofNoFill();
		int val = 128 + 127 * sinf(ofGetFrameNum() * 0.5);
		ofSetColor(val);

		ofDrawRectangle(squareSelectOrigin.x, squareSelectOrigin.y,
						ofGetMouseX() - squareSelectOrigin.x,
						ofGetMouseY() - squareSelectOrigin.y);
		ofFill();
		ofSetColor(255);
	}
}

    //--------------------------------------------------------------
    size_t Controller::findClosestWarp(const glm::vec2 & pos)
    {
        size_t warpIdx = -1;
        auto minDistance = std::numeric_limits<float>::max();

		std::map<int, float> distances;

        // Find warp and distance to closest control point.
        for (int i = this->warps.size() - 1; i >= 0; --i)
        {
            float candidateDist = 0;
            auto idx = this->warps[i]->findClosestControlPoint(pos, &candidateDist);
			distances[i] = candidateDist;
            if (candidateDist <= minDistance)
            {
                minDistance = candidateDist;
                warpIdx = i;
            }
        }

		//see if we have conflict points, where they both would be selected
		std::vector<int> candidateWarps;
		for(auto & it : distances){
			if(fabs(it.second - minDistance) < 1){
				candidateWarps.push_back(it.first);
			}
		}

		if(candidateWarps.size() > 1){ //we need to be which warps is closest to the mouse click
			float minDist = FLT_MAX;
			int warp = -1;
			for(int index : candidateWarps){
				auto ctr = warps[index]->getCenter();
				float d = ofDist(ctr.x, ctr.y, pos.x, pos.y);
				//ofLogNotice() << "warp: " << index << " ctr: " << ctr << " d: " << d;
				if(d < minDist){
					warp = index;
					minDist = d;
				}
			}
			if(warp >= 0) warpIdx = warp;
		}

		//ofLogNotice("Controller::findClosestWarp") << "final candidate for warp " << warpIdx;

        return warpIdx;
    }
    
	//--------------------------------------------------------------
	bool Controller::selectClosestControlPoint(const glm::vec2 & pos, bool extendSelection, float minDist)
	{

		bool found = false;

		// Select the closest control point and deselect all others depending on extendSelection.
		for (int i = this->warps.size() - 1; i >= 0; --i)
		{
			if (i == this->focusedIndex)
			{
                size_t temp = findClosestControlPoint(pos, minDist);
				if(temp >= 0 && temp < warps[i]->getNumControlPoints()){
					if(extendSelection){
						if(this->warps[i]->isControlPointSelected(temp)){
							this->warps[i]->deselectControlPoint(temp);
						}else{
							this->warps[i]->selectControlPoint(temp, extendSelection);
						}
					}else{
						this->warps[i]->selectControlPoint(temp, extendSelection);
						found = true;
					}
				}else{
					ofLogNotice("Controller") << "selectClosestControlPoint() no point closeby!";
				}
			}
		}
		return found;
	}
    
#pragma mark EDITING
    
    //--------------------------------------------------------------
    bool Controller::areWarpsInEditMode()
    {
        for(auto &warp : warps)
        {
            if(warp->isEditing())
            {
                return true;
            }
        }
        
        return false;
    }

	//--------------------------------------------------------------
	void Controller::turnEditingOn(){
		toggleEditing();
	}

	//--------------------------------------------------------------
	void Controller::turnEditingOff()
	{
		editingMode = false; 

		//Set all warps to false
		for (auto &warp : warps)
		{
			if (warp->isEditing())
				warp->toggleEditing();
		}
		focusedIndex = -1;
	}
    
    //--------------------------------------------------------------
    void Controller::toggleEditing()
    {
        editingMode = !editingMode;
        
		//Set all warps to false
		for(auto &warp : warps){
			warp->setEditing(false);
		}
		focusedIndex = -1;

        if(editingMode){
			if(warps.size()) warps[0]->setEditing(true);
			focusedIndex = 0;
		}
    }
    
#pragma mark MOUSE INTERACTIONS
    
    //--------------------------------------------------------------
    void Controller::setIgnoreMouseInteractions(bool _ignoreMouseInteractions)
    {
        ignoreMouseInteractions = _ignoreMouseInteractions;
    }

	//--------------------------------------------------------------
	void Controller::onMouseMoved(ofMouseEventArgs & args)
	{
        /*
         Removing hover affect to ensure that multiple control points are not clicked on at once.
         Found this feature to be difficult to use at large scale. Uncomment to add this funciton back in.
         */
		// Find and select closest control point.
		//this->selectClosestControlPoint(args);
         
	}

	//--------------------------------------------------------------
	void Controller::onMousePressed(ofMouseEventArgs & args)
	{
        //Global control of mouse interactions -- this is useful for if you want to have multiple
        //interctions modes (i.e. GUIs) overlayed ontop of the mouse interaction
        if(ignoreMouseInteractions) return;

		//Make sure the warps are in edit mode
		float minDist = ofGetHeight() / 5;

		if((ofGetKeyPressed('s') || ofGetKeyPressed('S'))){
			squareSelect = true;
			squareSelectOrigin = args;
			return;
		}

		//find closest warp and focus is
        if(ofGetKeyPressed(OF_KEY_LEFT_ALT)){
            //Toggle all warps to false
            for(auto &warp : warps) warp->setEditing(false);
            
            //Find selected warp and set it to edit mode
            focusedIndex = findClosestWarp(args);
			//ofLogNotice("Controller") << "closest warp = " << focusedIndex;
			if (focusedIndex >= 0 && focusedIndex < warps.size()){
            		warps[focusedIndex]->setEditing(true);
			}

			//if holding alt, select & drag an uselected point to drag it
			this->selectClosestControlPoint(args, ofGetKeyPressed(OF_KEY_SHIFT), minDist);

			if(ofGetKeyPressed(OF_KEY_SHIFT)){ // if also holding shift, select all pts
				if (focusedIndex >= 0 && focusedIndex < warps.size()){
					int n = warps[this->focusedIndex]->getNumControlPoints();
					for(int i = 0; i < n; i++){
						warps[this->focusedIndex]->selectControlPoint(i, true);
					}
				}
			}
        }

		int pt = findClosestControlPoint(args, minDist);
		if(pt >= 0){ //clicked on point
			if (focusedIndex >= 0 && focusedIndex < warps.size() && warps[focusedIndex]->isControlPointSelected(pt)){
				didClickOnCtrlPoint = true;
			}else{
				didClickOnCtrlPoint = false;
			}
		}else{
			didClickOnCtrlPoint = false;
		}

		//if holding alt, click & drag always drags selected points
		if(ofGetKeyPressed(OF_KEY_LEFT_ALT)) didClickOnCtrlPoint = true;

		mouseDown = true;
		mouseDownPos = ofVec2f(args.x, args.y);
		mouseDownTimestamp = ofGetElapsedTimef();

		if (focusedIndex >= 0 && focusedIndex < warps.size()){
			this->warps[this->focusedIndex]->handleCursorDown(args);
		}
	}

	//--------------------------------------------------------------
	void Controller::onMouseDragged(ofMouseEventArgs & args)
	{
        //Global control of mouse interactions -- this is useful for if you want to have multiple
        //interctions modes (i.e. GUIs) overlayed ontop of the mouse interaction
        if(ignoreMouseInteractions) return;

        //Make sure the warps are in edit mode
        if(!areWarpsInEditMode()) return;
        
        if (focusedIndex >= 0 && focusedIndex < warps.size()){
			if(didClickOnCtrlPoint){ //if we are clik-dragging on point, drag point
            		this->warps[this->focusedIndex]->handleCursorDrag(args);
			}
        }
    
	}

	//--------------------------------------------------------------
	void Controller::onMouseReleased(ofMouseEventArgs & args)
	{

		if(squareSelect){

			squareSelect = false;

			if (focusedIndex >= 0 && focusedIndex < warps.size()){
				ofRectangle r = ofRectangle(ofMap(squareSelectOrigin.x, 0, ofGetWidth(), 0, warps[focusedIndex]->getSize().x),
											ofMap(squareSelectOrigin.y, 0, ofGetHeight(), 0, warps[focusedIndex]->getSize().y),
											ofMap(ofGetMouseX() - squareSelectOrigin.x, 0, ofGetWidth(), 0, warps[focusedIndex]->getSize().x),
											ofMap(ofGetMouseY() - squareSelectOrigin.y, 0, ofGetHeight(), 0, warps[focusedIndex]->getSize().y)
											);

				int n = warps[focusedIndex]->getNumControlPoints();
				if(!ofGetKeyPressed(OF_KEY_SHIFT)){
					for(int i = 0; i < n; i++){
						warps[focusedIndex]->deselectControlPoint(i);
					}
				}
				//ofLogNotice() << "r: " << r;
				for(int i = 0; i < n; i++){
					auto pt = warps[focusedIndex]->getControlPoint(i) * warps[focusedIndex]->getSize();
					//ofLogNotice() << pt;
					if(r.inside(pt.x, pt.y)){
						warps[focusedIndex]->selectControlPoint(i, true);
					}
				}
			}

			return;
		}

		squareSelect = false;

		if(mouseDown){

			//Global control of mouse interactions -- this is useful for if you want to have multiple
			//interctions modes (i.e. GUIs) overlayed ontop of the mouse interaction
			if(ignoreMouseInteractions) return;

			//Make sure the warps are in edit mode
			if(!areWarpsInEditMode()) return;

			float travelDist = mouseDownPos.distance(ofVec2f(args.x, args.y));
			float downTime = ofGetElapsedTimef() - mouseDownTimestamp;
			didClickOnCtrlPoint = false;
			mouseDown = false;

			if(travelDist < 5 && downTime < 0.3){
				float minDist = ofGetHeight() / 5.0f;
				this->selectClosestControlPoint(args, ofGetKeyPressed(OF_KEY_SHIFT), minDist);
			}
		}
    }

#pragma mark KEY INTERACTIONS
    
	//--------------------------------------------------------------
	void Controller::onKeyPressed(ofKeyEventArgs & args)
	{
		if (args.key == 'w')
		{
            toggleEditing();
            
            /*
             Removed this to make editing mode a clickable interaction
             
			for (auto warp : this->warps)
			{
				warp->toggleEditing();
			}
             */
		}
        else
			if (focusedIndex >= 0 && focusedIndex < warps.size()){

			auto warp = this->warps[this->focusedIndex];

			if (args.key == '-')
			{
				warp->setBrightness(MAX(0.0f, warp->getBrightness() - 0.01f));
			}
			else if (args.key == '+')
			{
				warp->setBrightness(MIN(1.0f, warp->getBrightness() + 0.01f));
			}
			else if (args.key == 'r')
			{
				warp->reset();
			}
			else if (args.key == OF_KEY_TAB)
			{
				// Select the next of previous (+ SHIFT) control point.
				size_t nextIndex;
				auto selectedIndex = warp->getSelectedControlPoint();
				if (ofGetKeyPressed(OF_KEY_SHIFT))
				{
					if (selectedIndex == 0)
					{
						nextIndex = warp->getNumControlPoints() - 1;
					}
					else
					{
						nextIndex = selectedIndex - 1;
					}
				}
				else
				{
					nextIndex = (selectedIndex + 1) % warp->getNumControlPoints();
				}
				warp->selectControlPoint(nextIndex, false);
			}
			//else if (args.key == OF_KEY_UP || args.key == OF_KEY_DOWN || args.key == OF_KEY_LEFT || args.key == OF_KEY_RIGHT)
			// Can't use OF_KEY_XX, see https://github.com/openframeworks/openFrameworks/issues/5948
			else if (args.keycode == GLFW_KEY_UP || args.keycode == GLFW_KEY_DOWN || args.keycode == GLFW_KEY_LEFT || args.keycode == GLFW_KEY_RIGHT)
			{
				auto step = ofGetKeyPressed(OF_KEY_SHIFT) ? 10.0f : 0.5f;
				auto shift = glm::vec2(0.0f);
				if (args.key == OF_KEY_UP)
				{
					shift.y = -step / (float)ofGetHeight();
				}
				else if (args.key == OF_KEY_DOWN)
				{
					shift.y = step / (float)ofGetHeight();
				}
				else if (args.key == OF_KEY_LEFT)
				{
					shift.x = -step / (float)ofGetWidth();
				}
				else
				{
					shift.x = step / (float)ofGetWidth();
				}
				
                warp->moveControlPoints(shift);
                
			}
			else if (args.key == OF_KEY_F9)
			{
				warp->rotateCounterclockwise();
			}
			else if (args.key == OF_KEY_F10)
			{
				warp->rotateClockwise();
			}
			else if (args.key == OF_KEY_F11)
			{
				warp->flipHorizontal();
			}
			else if (args.key == OF_KEY_F12)
			{
				warp->flipVertical();
			}
			else if (warp->getType() == WarpBase::TYPE_BILINEAR || warp->getType() == WarpBase::TYPE_PERSPECTIVE_BILINEAR)
			{
				// The rest of the controls only apply to Bilinear warps.
				auto warpBilinear = std::dynamic_pointer_cast<WarpBilinear>(warp);
				if (warpBilinear)
				{
					if (args.key == OF_KEY_F1)
					{
						// Reduce the number of horizontal control points.
						if (ofGetKeyPressed(OF_KEY_SHIFT))
						{
							warpBilinear->setNumControlsX(warpBilinear->getNumControlsX() - 1);
						}
						else
						{
							warpBilinear->setNumControlsX((warpBilinear->getNumControlsX() + 1) / 2);
						}
					}
					else if (args.key == OF_KEY_F2)
					{
						// Increase the number of horizontal control points.
						if (ofGetKeyPressed(OF_KEY_SHIFT))
						{
							warpBilinear->setNumControlsX(warpBilinear->getNumControlsX() + 1);
						}
						else
						{
							warpBilinear->setNumControlsX(warpBilinear->getNumControlsX() * 2 - 1);
						}
					}
					else if (args.key == OF_KEY_F3)
					{
						// Reduce the number of vertical control points.
						if (ofGetKeyPressed(OF_KEY_SHIFT))
						{
							warpBilinear->setNumControlsY(warpBilinear->getNumControlsY() - 1);
						}
						else
						{
							warpBilinear->setNumControlsY((warpBilinear->getNumControlsY() + 1) / 2);
						}
					}
					else if (args.key == OF_KEY_F4)
					{
						// Increase the number of vertical control points.
						if (ofGetKeyPressed(OF_KEY_SHIFT))
						{
							warpBilinear->setNumControlsY(warpBilinear->getNumControlsY() + 1);
						}
						else
						{
							warpBilinear->setNumControlsY(warpBilinear->getNumControlsY() * 2 - 1);
						}
					}
					else if (args.key == OF_KEY_F5)
					{
						warpBilinear->decreaseResolution();
					}
					else if (args.key == OF_KEY_F6)
					{
						warpBilinear->increaseResolution();
					}
					else if (args.key == OF_KEY_F7)
					{
						warpBilinear->setAdaptive(!warpBilinear->getAdaptive());
					}
					else if (args.key == 'm')
					{
						warpBilinear->setLinear(!warpBilinear->getLinear());
					}
				}
			}
		}
        
        
       
	}

	//--------------------------------------------------------------
	void Controller::onKeyReleased(ofKeyEventArgs & args)
    {
    
    }

	//--------------------------------------------------------------
	void Controller::onWindowResized(ofResizeEventArgs & args)
	{
		for (auto warp : this->warps)
		{
			warp->handleWindowResize(args.width, args.height);
		}
	}
    
}
