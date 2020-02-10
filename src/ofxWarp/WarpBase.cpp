#include "WarpBase.h"

#include "ofPolyline.h"

namespace ofxWarp
{
	//--------------------------------------------------------------
	std::filesystem::path WarpBase::shaderPath = std::filesystem::path("shaders") / "ofxWarp";

	//--------------------------------------------------------------
	void WarpBase::setShaderPath(const std::filesystem::path shaderPath)
	{
		WarpBase::shaderPath = shaderPath;
	}

	//--------------------------------------------------------------
	WarpBase::WarpBase(Type type)
		: type(type)
		, editing(false)
		, dirty(true)
		, brightness(1.0f)
		, width(640.0f)
		, height(480.0f)
		, numControlsX(2)
		, numControlsY(2)
		, selectedIndex(-1)
		, selectedTime(0.0f)
		, luminance(0.5f)
		, gamma(1.0f)
		, exponent(2.0f)
		, edges(0.0f)
	{
		this->windowSize = glm::vec2(ofGetWidth(), ofGetHeight());
	}
	
	//--------------------------------------------------------------
	WarpBase::~WarpBase()
	{}

	//--------------------------------------------------------------
	WarpBase::Type WarpBase::getType() const
	{
		return this->type;
	}

	//--------------------------------------------------------------
	void WarpBase::serialize(nlohmann::json & json)
	{
		// Main parameters.
		json["type"] = this->type;
		json["brightness"] = this->brightness;

		// Warp parameters.
		{
			auto & jsonWarp = json["warp"];

			jsonWarp["columns"] = this->numControlsX;
			jsonWarp["rows"] = this->numControlsY;
            
            jsonWarp["width"] = this->width;
            jsonWarp["height"] = this->height;
            
			std::vector<std::string> points;
			for (auto & controlPoint : this->controlPoints)
			{
				std::ostringstream oss;
				oss << controlPoint;
				points.push_back(oss.str());
			}
			jsonWarp["control points"] = points;
		}

		// Blend parameters.
		{
			auto & jsonBlend = json["blend"];

			jsonBlend["exponent"] = this->exponent;

			std::ostringstream oss;
			oss << this->edges;
			jsonBlend["edges"] = oss.str();

			oss.str("");
			oss << this->gamma;
			jsonBlend["gamma"] = oss.str();

			oss.str("");
			oss << this->luminance;
			jsonBlend["luminance"] = oss.str();
		}
	}
	
	//--------------------------------------------------------------
	void WarpBase::deserialize(const nlohmann::json & json)
	{
		// Main parameters.
		int typeAsInt = json["type"];
		this->type = (Type)typeAsInt;
		this->brightness = json["brightness"];

		// Warp parameters.
		{
			const auto & jsonWarp = json["warp"];

			this->numControlsX = jsonWarp["columns"];
			this->numControlsY = jsonWarp["rows"];
            
			this->controlPoints.clear();
            
            this->width =  jsonWarp["width"] ;
            this->height = jsonWarp["height"];
        
			for (const auto & jsonPoint : jsonWarp["control points"])
			{
				glm::vec2 controlPoint;
				std::istringstream iss;
				iss.str(jsonPoint);
				iss >> controlPoint;
				this->controlPoints.push_back(controlPoint);
			}
            
		}

		assignNewControlPoints(this->controlPoints); 
       
		
        
		// Blend parameters.
		{
			const auto & jsonBlend = json["blend"];

			this->exponent = jsonBlend["exponent"];

			{
				std::istringstream iss;
				iss.str(jsonBlend["edges"]);
				iss >> this->edges;
			}
			{
				std::istringstream iss;
				iss.str(jsonBlend["gamma"]);
				iss >> this->gamma;
			}
			{
				std::istringstream iss;
				iss.str(jsonBlend["luminance"]);
				iss >> this->luminance;
			}
		}

		this->dirty = true;

	}

	//--------------------------------------------------------------
	void WarpBase::setEditing(bool editing)
	{
		this->editing = editing;
	}
	
	//--------------------------------------------------------------
	void WarpBase::toggleEditing()
	{
		this->setEditing(!this->editing);
        
        //Turn off all control points
        for(int i = 0; i < selectedIndices.size(); i++)
        {
            
            selectedIndices[i] = 0;
        }
        
	}

	glm::vec2 WarpBase::getCenter(){
		if(controlPoints.size()){
			double avgx, avgy;
			for(int i = 0; i < controlPoints.size(); i++){
				auto pt = this->getControlPoint(i);
				avgx += pt.x * windowSize.x / controlPoints.size();
				avgy += pt.y * windowSize.y / controlPoints.size();
			}
			return glm::vec2(avgx, avgy);
		}else{
			return glm::vec2();
		}
	}
	//--------------------------------------------------------------
	bool WarpBase::isEditing() const
	{
        return this->editing;

	}

	//--------------------------------------------------------------
	void WarpBase::setWidth(float width)
	{
		this->setSize(width, this->height);
	}

	//--------------------------------------------------------------
	float WarpBase::getWidth() const
	{
		return this->width;
	}

	//--------------------------------------------------------------
	void WarpBase::setHeight(float height)
	{
		this->setSize(this->width, height);
	}

	//--------------------------------------------------------------
	float WarpBase::getHeight() const
	{
		return this->height;
	}

	//--------------------------------------------------------------
	void WarpBase::setSize(float width, float height)
	{
		this->width = width;
		this->height = height;
		this->dirty = true;
	}
	
	//--------------------------------------------------------------
	void WarpBase::setSize(const glm::vec2 & size)
	{
		this->setSize(size.x, size.y);
	}
	
	//--------------------------------------------------------------
	glm::vec2 WarpBase::getSize() const
	{
		return glm::vec2(this->width, this->height);
	}

	//--------------------------------------------------------------
	ofRectangle WarpBase::getBounds() const
	{
		return ofRectangle(0, 0, this->width, this->height);
	}

	//--------------------------------------------------------------
	void WarpBase::setBrightness(float brightness)
	{
		this->brightness = brightness;
	}
	
	//--------------------------------------------------------------
	float WarpBase::getBrightness() const
	{
		return this->brightness;
	}

	//--------------------------------------------------------------
	void WarpBase::setLuminance(float luminance)
	{
		this->luminance = glm::vec3(luminance);
	}
	
	//--------------------------------------------------------------
	void WarpBase::setLuminance(float red, float green, float blue)
	{
		this->luminance = glm::vec3(red, green, blue);
	}

	//--------------------------------------------------------------
	void WarpBase::setLuminance(const glm::vec3 & rgb)
	{
		this->luminance = rgb;
	}
	
	//--------------------------------------------------------------
	const glm::vec3 & WarpBase::getLuminance() const
	{
		return this->luminance;
	}

	//--------------------------------------------------------------
	void WarpBase::setGamma(float gamma)
	{
		this->gamma = glm::vec3(gamma);
	}
	
	//--------------------------------------------------------------
	void WarpBase::setGamma(float red, float green, float blue)
	{
		this->gamma = glm::vec3(red, green, blue);
	}

	//--------------------------------------------------------------
	void WarpBase::setGamma(const glm::vec3 & rgb)
	{
		this->gamma = rgb;
	}
	
	//--------------------------------------------------------------
	const glm::vec3 & WarpBase::getGamma() const
	{
		return this->gamma;
	}

	//--------------------------------------------------------------
	void WarpBase::setExponent(float exponent)
	{
		this->exponent = exponent;
	}
	
	//--------------------------------------------------------------
	float WarpBase::getExponent() const
	{
		return this->exponent;
	}

	//--------------------------------------------------------------
	void WarpBase::setEdges(float left, float top, float right, float bottom)
	{
		this->setEdges(glm::vec4(left, top, right, bottom));
	}
	
	//--------------------------------------------------------------
	void WarpBase::setEdges(const glm::vec4 & edges)
	{
		this->edges.x = ofClamp(edges.x * 0.5f, 0.0f, 1.0f);
		this->edges.y = ofClamp(edges.y * 0.5f, 0.0f, 1.0f);
		this->edges.z = ofClamp(edges.z * 0.5f, 0.0f, 1.0f);
		this->edges.w = ofClamp(edges.w * 0.5f, 0.0f, 1.0f);
	}
	
	//--------------------------------------------------------------
	glm::vec4 WarpBase::getEdges() const
	{
		return this->edges * 2.0f;
	}

	//--------------------------------------------------------------
	void WarpBase::draw(const ofTexture & texture)
	{
		this->draw(texture, ofRectangle(0, 0, texture.getWidth(), texture.getHeight()), this->getBounds());
	}
	
	//--------------------------------------------------------------
	void WarpBase::draw(const ofTexture & texture, const ofRectangle & srcBounds)
	{
		this->draw(texture, srcBounds, this->getBounds());
	}

	//--------------------------------------------------------------
	void WarpBase::draw(const ofTexture & texture, const ofRectangle & srcBounds, const ofRectangle & dstBounds)
	{
		this->drawTexture(texture, srcBounds, dstBounds);
		this->drawControls();
	}
	
	//--------------------------------------------------------------
	bool WarpBase::clip(ofRectangle & srcBounds, ofRectangle & dstBounds) const
	{
		bool clipped = false;

		glm::vec4 srcVec = glm::vec4(srcBounds.getMinX(), srcBounds.getMinY(), srcBounds.getMaxX(), srcBounds.getMaxY());
		glm::vec4 dstVec = glm::vec4(dstBounds.getMinX(), dstBounds.getMinY(), dstBounds.getMaxX(), dstBounds.getMaxY());
		
		float x1 = dstVec.x / this->width;
		float x2 = dstVec.z / this->width;
		float y1 = dstVec.y / this->height;
		float y2 = dstVec.w / this->height;

		if (x1 < 0.0f) 
		{
			dstVec.x = 0.0f;
			srcVec.x -= (x1 * srcBounds.getWidth());
			clipped = true;
		}
		else if (x1 > 1.0f) 
		{
			dstVec.x = this->width;
			srcVec.x -= ((1.0f / x1) * srcBounds.getWidth());
			clipped = true;
		}

		if (x2 < 0.0f) 
		{
			dstVec.z = 0.0f;
			srcVec.z -= (x2 * srcBounds.getWidth());
			clipped = true;
		}
		else if (x2 > 1.0f) {
			dstVec.z = this->width;
			srcVec.z -= ((1.0f / x2) * srcBounds.getWidth());
			clipped = true;
		}

		if (y1 < 0.0f) 
		{
			dstVec.y = 0.0f;
			srcVec.y -= (y1 * srcBounds.getHeight());
			clipped = true;
		}
		else if (y1 > 1.0f) 
		{
			dstVec.y = this->height;
			srcVec.y -= ((1.0f / y1) * srcBounds.getHeight());
			clipped = true;
		}

		if (y2 < 0.0f) {
			dstVec.w = 0.0f;
			srcVec.w -= (y2 * srcBounds.getHeight());
			clipped = true;
		}
		else if (y2 > 1.0f) {
			dstVec.w = this->height;
			srcVec.w -= ((1.0f / y2) * srcBounds.getHeight());
			clipped = true;
		}

		srcBounds.set(srcVec.x, srcVec.y, srcVec.z - srcVec.x, srcVec.w - srcVec.y);
		dstBounds.set(dstVec.x, dstVec.y, dstVec.z - dstVec.x, dstVec.w - dstVec.y);

		return clipped;
	}

#pragma mark CONTROL POINTS
    
	//--------------------------------------------------------------
	glm::vec2 WarpBase::getControlPoint(size_t index) const
	{
		if (index >= this->controlPoints.size()) 
		{
			ofLogError() << "index is out of bounds of controlPoints.size(): " << controlPoints.size(); 

			return glm::vec2(0.0f, 0.0f);
		}

		return this->controlPoints[index];
	}

	//--------------------------------------------------------------
	void WarpBase::setControlPoint(size_t index, const glm::vec2 & pos)
	{
		if (index >= this->controlPoints.size()) return;

		this->controlPoints[index] = pos;
		this->dirty = true;
	}

	//--------------------------------------------------------------
	void WarpBase::moveControlPoint(size_t index, const glm::vec2 & shift)
	{
		if (index >= this->controlPoints.size()) return;

		this->controlPoints[index] += shift;
		this->dirty = true;
	}
    
    //--------------------------------------------------------------
    void WarpBase::moveControlPoints(const glm::vec2 & shift)
    {
        
        for(int i = 0; i < selectedIndices.size(); i++)
        {
            if(selectedIndices[i] == 1)
            {
                this->controlPoints[i] += shift;
                this->dirty = true;
            }
        }
    
    }

	//--------------------------------------------------------------
	size_t WarpBase::getNumControlPoints() const
	{
		return this->controlPoints.size();
	}

	//--------------------------------------------------------------
	size_t WarpBase::getSelectedControlPoint() const
	{
		return this->selectedIndex;
	}

	bool WarpBase::isControlPointSelected(size_t index){
		return index == this->selectedIndex;
	};

	//--------------------------------------------------------------
	void WarpBase::selectControlPoint(size_t index, bool extendSelection)
	{

        if(index >= this->controlPoints.size() || index >= this->selectedIndices.size())
        {
            return;
        }

		if(!extendSelection){
			for(auto & sel : selectedIndices) sel = 0;
		}

        selectedIndices[index] = 1;
        this->selectedTime = ofGetElapsedTimef();
	}



	//--------------------------------------------------------------
	void WarpBase::deselectControlPoints()
	{

        for(auto &index : selectedIndices)
        {
            index = 0;
        }
	}
    
    void WarpBase::deselectControlPoint(size_t index)
    {
        if(index >= selectedIndices.size())
            return;
        
        selectedIndices[index] = 0;
    }

	//--------------------------------------------------------------
	size_t WarpBase::findClosestControlPoint(const glm::vec2 & pos, float * distance) const
	{
		size_t index;
		auto minDistance = std::numeric_limits<float>::max();

		for (auto i = 0; i < this->controlPoints.size(); ++i)
		{
			auto candidate = glm::distance(pos, this->getControlPoint(i) * this->windowSize);
			if (candidate < minDistance)
			{
				minDistance = candidate;
				index = i;
			}
		}

		*distance = minDistance;
		return index;
	}

	//--------------------------------------------------------------
	size_t WarpBase::getNumControlsX() const
	{
		return this->numControlsX;
	}
	
	//--------------------------------------------------------------
	size_t WarpBase::getNumControlsY() const
	{
		return this->numControlsY;
	}

	//--------------------------------------------------------------
	void WarpBase::queueControlPoint(const glm::vec2 & pos, bool selected, bool attached)
	{
		if (selected && attached) 
		{
			queueControlPoint(pos, ofFloatColor(0.0f, 0.8f, 0.0f));
		}
		else if (selected) 
		{
			auto scale = 0.9f + 0.2f * sinf(6.0f * (ofGetElapsedTimef() - this->selectedTime));
			//queueControlPoint(pos, ofFloatColor(0.9f, 0.9f, 0.9f), scale);
            queueControlPoint(pos, ofFloatColor(0.0f, 1.0f, 0.0f), scale);
		}
		else if (attached) 
		{
			queueControlPoint(pos, ofFloatColor(0.0f, 0.4f, 0.0f));
		}
		else 
		{
			queueControlPoint(pos, ofFloatColor(0.4f, 0.4f, 0.4f));
		}
	}
	
	//--------------------------------------------------------------
	void WarpBase::queueControlPoint(const glm::vec2 & pos, const ofFloatColor & color, float scale)
	{
		if (this->controlData.size() < MAX_NUM_CONTROL_POINTS)
		{
			this->controlData.emplace_back(ControlData(pos, color, scale));
		}
	}

	//--------------------------------------------------------------
	void WarpBase::setupControlPoints()
	{
		if (this->controlMesh.getVertices().empty())
		{
			// Set up the vbo mesh.
			ofPolyline unitCircle;
			unitCircle.arc(glm::vec3(0.0f), 1.0f, 1.0f, 0.0f, 360.0f, 18);
			const auto & circlePoints = unitCircle.getVertices();
			static const auto radius = 15.0f;
			static const auto halfVec = glm::vec2(0.5f);
			this->controlMesh.clear();
			this->controlMesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
			this->controlMesh.setUsage(GL_STATIC_DRAW);
			this->controlMesh.addVertex(glm::vec3(0.0f));
			this->controlMesh.addTexCoord(halfVec);
			for (auto & pt : circlePoints)
			{
				this->controlMesh.addVertex(pt * radius);
                this->controlMesh.addTexCoord(glm::vec2(pt) * 0.5f + halfVec);
			}

			// Set up per-instance data to the vbo.
			std::vector<ControlData> instanceData;
			instanceData.resize(MAX_NUM_CONTROL_POINTS);

			this->controlMesh.getVbo().setAttributeData(INSTANCE_POS_SCALE_ATTRIBUTE, (float *)&instanceData[0].pos, 4, instanceData.size(), GL_STREAM_DRAW, sizeof(ControlData));
			this->controlMesh.getVbo().setAttributeDivisor(INSTANCE_POS_SCALE_ATTRIBUTE, 1);
			this->controlMesh.getVbo().setAttributeData(INSTANCE_COLOR_ATTRIBUTE, (float *)&instanceData[0].color, 4, instanceData.size(), GL_STREAM_DRAW, sizeof(ControlData));
			this->controlMesh.getVbo().setAttributeDivisor(INSTANCE_COLOR_ATTRIBUTE, 1);
		}

		if (!this->controlShader.isLoaded())
		{
			// Load the shader.
			this->controlShader.setupShaderFromFile(GL_VERTEX_SHADER, WarpBase::shaderPath / "ControlPoint.vert");
			this->controlShader.setupShaderFromFile(GL_FRAGMENT_SHADER, WarpBase::shaderPath / "ControlPoint.frag");
			this->controlShader.bindAttribute(INSTANCE_POS_SCALE_ATTRIBUTE, "iPositionScale");
			this->controlShader.bindAttribute(INSTANCE_COLOR_ATTRIBUTE, "iColor");
			this->controlShader.bindDefaults();
			this->controlShader.linkProgram();
		}
	}

	//--------------------------------------------------------------
	void WarpBase::drawControlPoints()
	{
		this->setupControlPoints();

		if (!this->controlData.empty())
		{
			this->controlMesh.getVbo().updateAttributeData(INSTANCE_POS_SCALE_ATTRIBUTE, (float *)&this->controlData[0].pos, this->controlData.size());
			this->controlMesh.getVbo().updateAttributeData(INSTANCE_COLOR_ATTRIBUTE, (float *)&this->controlData[0].color, this->controlData.size());
		
			this->controlShader.begin();
			{
				this->controlMesh.drawInstanced(OF_MESH_FILL, this->controlData.size());
			}
			this->controlShader.end();
		}

		this->controlData.clear();
	}
    
    //--------------------------------------------------------------
    void WarpBase::assignNewControlPoints(std::vector<glm::vec2> _controlPoints)
    {
        controlPoints = _controlPoints;
        
        //Clear all selected indices
		selectedIndices.clear();
        
        //Add new indices
        for(int i = 0; i < controlPoints.size(); i++){
            selectedIndices.push_back(0);
            selectedOffsets.push_back(glm::vec2());
        }
        
    }
	
	//--------------------------------------------------------------
	bool WarpBase::handleCursorDown(const glm::vec2 & pos)
	{
        
        if (!this->editing) return false;
        
        for(int i = 0 ; i < selectedIndices.size() ; i++)
        {
            if(selectedIndices[i] == 1)
            {
                // Calculate offset by converting control point from normalized to screen space.
                glm::vec2 screenPoint = (this->getControlPoint(i) * this->windowSize);
                selectedOffsets[i] = pos - screenPoint;
            }
        }
        
        return true;
	}
	
	//--------------------------------------------------------------
	bool WarpBase::handleCursorDrag(const glm::vec2 & pos)
	{
        
        if (!this->editing) return false;
        
        for(int i = 0 ; i < selectedIndices.size() ; i++)
        {
            if(selectedIndices[i] == 1)
            {
                // Set control point in normalized space.
                glm::vec2 screenPoint = pos - selectedOffsets[i];
                this->setControlPoint(i, screenPoint / this->windowSize);
            }
        }
        
        this->dirty = true;
        return true;
	}

	//--------------------------------------------------------------
	bool WarpBase::handleWindowResize(int width, int height)
	{
		this->windowSize = glm::vec2(width, height);
		this->dirty = true;

		return true;
	}
}
