#pragma once

#include "ofEvents.h"
#include "WarpBase.h"

namespace ofxWarp
{
	class Controller
	{
	public:
		Controller();
		~Controller();


        void removeListeners();

		//! write a settings json file
		bool saveSettings(const std::string & filePath);
		//! read a settings json file
		bool loadSettings(const std::string & filePath);
		
		//! serialize the list of warps to a json file
		void serialize(nlohmann::json & json);
		//! deserialize the list of warps from a json file
		void deserialize(const nlohmann::json & json);

		//! build and add a new warp of the specified type
		template<class Type>
		inline std::shared_ptr<Type> buildWarp()
		{
			auto warp = std::make_shared<Type>();
			this->warps.push_back(warp);

			return warp;
		}

		//! add the warp to the list
		bool addWarp(std::shared_ptr<WarpBase> warp);
		//! remove the warp from the list
		bool removeWarp(std::shared_ptr<WarpBase> warp);

		//! return the list of warps
		std::vector<std::shared_ptr<WarpBase>> & getWarps();
		//! return the warp at the specified index
		std::shared_ptr<WarpBase> getWarp(size_t index) const;
		//! return the number of warps
		size_t getNumWarps() const;

		//! handle mouseMoved events for multiple warps
		void onMouseMoved(ofMouseEventArgs & args);
        void onMouseMoved(int x, int y);
		//! handle mousePressed events for multiple warps
		void onMousePressed(ofMouseEventArgs & args);
        void onMousePressed(int x, int y);
		//! handle mouseDragged events for multiple warps
		void onMouseDragged(ofMouseEventArgs & args);
        void onMouseDragged(int x, int y);
		//! handle mouseReleased events for multiple warps
		void onMouseReleased(ofMouseEventArgs & args);
        void onMouseReleased(int x, int y);

		//! handle keyPressed events for multiple warps
		void onKeyPressed(ofKeyEventArgs & args);
        void onKeyPressed(int key);
		//! handle keyReleased events for multiple warps
		void onKeyReleased(ofKeyEventArgs & args);
        void onKeyReleased(int key);

		//! handle windowResized events for multiple warps
		void onWindowResized(ofResizeEventArgs & args);
        void onWindowResized(int w, int h);

	protected:
		//! check all warps and select the closest control point
		void selectClosestControlPoint(const glm::vec2 & pos);

	protected:
		std::vector<std::shared_ptr<WarpBase>> warps;

		size_t focusedIndex;
	};
}
