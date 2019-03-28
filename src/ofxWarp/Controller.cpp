#include "Controller.h"

#include "WarpBilinear.h"
#include "WarpPerspective.h"
#include "WarpPerspectiveBilinear.h"

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
    size_t Controller::findClosestControlPoint(const glm::vec2 & pos)
    {
        size_t pointIdx = -1;
        auto distance = std::numeric_limits<float>::max();
        
        // Find warp and distance to closest control point.
        for (int i = this->warps.size() - 1; i >= 0; --i)
        {
            float candidate;
            auto idx = this->warps[i]->findClosestControlPoint(pos, &candidate);
            if (candidate < distance && this->warps[i]->isEditing())
            {
                distance = candidate;
                pointIdx = idx;
            }
        }
        
        return pointIdx;
    }
    
    //--------------------------------------------------------------
    size_t Controller::findClosestWarp(const glm::vec2 & pos)
    {
        size_t warpIdx = -1;
        auto distance = std::numeric_limits<float>::max();
        
        // Find warp and distance to closest control point.
        for (int i = this->warps.size() - 1; i >= 0; --i)
        {
            float candidate;
            auto idx = this->warps[i]->findClosestControlPoint(pos, &candidate);
            if (candidate < distance)
            {
                distance = candidate;
                warpIdx = i;
            }
        }
        
        return warpIdx;
    }
    
	//--------------------------------------------------------------
	void Controller::selectClosestControlPoint(const glm::vec2 & pos)
	{
        
		// Select the closest control point and deselect all others.
		for (int i = this->warps.size() - 1; i >= 0; --i)
		{
			if (i == this->focusedIndex)
			{
                //BEFORE:
                //focusedIndexControlPoint = findClosestControlPoint(pos);
				//this->warps[i]->selectControlPoint(focusedIndexControlPoint);
                
                size_t temp =  findClosestControlPoint(pos);
                //focusedIndexControlPoints.push_back(temp);
                
                this->warps[i]->selectControlPoint(temp);
			}
		}
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
    bool Controller::toggleEditing()
    {
        editingMode = !editingMode;
        
        if(!editingMode)
        {
            //Set all warps to false
            for(auto &warp : warps)
            {
                if(warp->isEditing())
                    warp->toggleEditing();
            }
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
        
        
        if(ofGetKeyPressed(OF_KEY_LEFT_ALT) && editingMode)
        {
            //Toggle all warps to false
            for(auto &warp : warps) warp->setEditing(false);
            
            //Find selected warp
            focusedIndex = findClosestWarp(args);
            warps[focusedIndex]->toggleEditing();
    
        }
        
        //Make sure the warps are in edit mode
        if(!areWarpsInEditMode()) return;
        
        this->selectClosestControlPoint(args);
        this->warps[this->focusedIndex]->handleCursorDown(args);
        
       
	}

	//--------------------------------------------------------------
	void Controller::onMouseDragged(ofMouseEventArgs & args)
	{
        //Global control of mouse interactions -- this is useful for if you want to have multiple
        //interctions modes (i.e. GUIs) overlayed ontop of the mouse interaction
        if(ignoreMouseInteractions) return;
        
        //Make sure the warps are in edit mode
        if(!areWarpsInEditMode()) return;
        
        if (this->focusedIndex < this->warps.size())
        {
            this->warps[this->focusedIndex]->handleCursorDrag(args);
        }
    
	}

	//--------------------------------------------------------------
	void Controller::onMouseReleased(ofMouseEventArgs & args)
	{
        
        //Global control of mouse interactions -- this is useful for if you want to have multiple
        //interctions modes (i.e. GUIs) overlayed ontop of the mouse interaction
        if(ignoreMouseInteractions) return;
        
        //Make sure the warps are in edit mode
        if(!areWarpsInEditMode()) return;

        if (ofGetKeyPressed(OF_KEY_SHIFT))
        {
            
            //Unselect control point
            size_t temp = findClosestControlPoint(args);
            warps[focusedIndex]->deselectControlPoint(temp);

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
        else if (this->focusedIndex < this->warps.size())
		{
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
				warp->selectControlPoint(nextIndex);
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
				
                //BEFORE
                //warp->moveControlPoint(warp->getSelectedControlPoint(), shift);
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
