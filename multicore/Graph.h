#ifndef Graph_h__
#define Graph_h__

#include <DXLib/Common.h>
#include <DXLib/SpriteRenderer.h>
#include <DXLib/ContentManager.h>
#include <DXLib/Timer.h>

#include <vector>
#include <string>
#include <map>
#include <queue>
#include <deque>
#include <limits>

struct LegendIndex
{
	LegendIndex()
		: position(0.0f, 0.0f)
		, color(0.0f, 0.0f, 0.0f)
	{}
	LegendIndex(std::string name, DirectX::XMFLOAT2 position, DirectX::XMFLOAT3 color)
		: name(name)
		, position(position)
		, color(color)
	{}

	std::string name;
	DirectX::XMFLOAT2 position;
	DirectX::XMFLOAT3 color;
};

namespace
{
	struct LineVertex 
	{
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT4 color;
		float dashValue;
		uint8_t padding[4];

		LineVertex()
			: position(0.0f, 0.0f)
			, color(1.0f, 1.0f, 1.0f, 1.0f)
			, dashValue(0)
		{}
		LineVertex(DirectX::XMFLOAT2 position, DirectX::XMFLOAT4 color, float dashValue)
			: position(position)
			, color(color)
			, dashValue(dashValue)
		{}
	};

	class Track
	{
	public:
		//Maximum points when calculating average
		const int MAX_AVERAGE_POINTS = 25;

		Track(float xResolution, DirectX::XMFLOAT3 color)
			: color(color)
			, maxValues(0)
			, addedValues(0)
			, xResolution(xResolution)
			, maxValue(std::numeric_limits<float>::min())
			, minValue(std::numeric_limits<float>::max())
		{}

		virtual ~Track() = default;

		virtual void AddValue(float value) = 0;
		virtual float CalculateAverage()
		{
			float average = 0.0f;

			int valueRange = std::min(static_cast<int>(std::ceil(values.size() * 0.25f)), MAX_AVERAGE_POINTS);
			if(values.size() >= valueRange)
			{
				int endDistance = valueRange;

				if(values.size() - endDistance <= 0)
					endDistance = static_cast<int>(values.size());

				for(auto iter = values.end() - endDistance, end = values.end(); iter != end; ++iter)
					average += *iter;

				average /= static_cast<float>(valueRange);
			}

			return average;
		}

		void Clear()
		{
			values.clear();
			addedValues = 0;
			maxValue = 0;
			minValue = 0;
		}

		DirectX::XMFLOAT3 color;
		std::deque<float> values;

		int maxValues;
		int addedValues;
		float xResolution;
		float maxValue;
		float minValue;
		float lastValue;
	};

	class TrackPerSecond
		: public Track
	{
	public:
		TrackPerSecond(float addTime, float xResolution, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f))
			: Track(xResolution, color)
			, addTime(addTime)
		{
			timer.Start();
		}

		void AddValue(float value) override
		{
			++addedValues;
			lastValue += value;

			if(timer.GetTimeMillisecondsFraction() * 0.001f >= addTime)
			{
				values.push_back(lastValue / static_cast<float>(addedValues));

				//The first time a value is added it need to execute both these branches.
				//Thus, no else
				if(values.back() < minValue)
					minValue = values.back();
				if(values.back() > maxValue)
					maxValue = values.back();

				timer.Reset();
				addedValues = 0;
				lastValue = 0.0f;
			}

			if(values.size() > maxValues)
			{
				values.erase(values.begin(), values.begin() + (values.size() - maxValues));

				maxValue = std::numeric_limits<float>::min();
				minValue = std::numeric_limits<float>::max();

				for(int i = 0, end = static_cast<int>(values.size()); i < end; ++i)
				{
					if(values[i] > maxValue)
						maxValue = values[i];
					if(values[i] < minValue)
						minValue = values[i];
				}
			}
		}

		Timer timer;
		float addTime;
	};

	class TrackPerAdd
		: public Track
	{
	public:
		TrackPerAdd(int valuesToAverage, float xResolution, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f))
			: Track(xResolution, color)
			, valuesToAverage(valuesToAverage)
		{
			values.emplace_back(0.0f);
		}

		void AddValue(float value) override
		{
			++addedValues;
			lastValue += value;

			if(addedValues == valuesToAverage)
			{
				//The first time a value is added it need to execute both these branches.
				//Thus, no else
				values.push_back(lastValue / static_cast<float>(valuesToAverage));

				if(values.back() < minValue)
					minValue = values.back();
				if(values.back() > maxValue)
					maxValue = values.back();

				addedValues = 0;
				lastValue = 0.0f;
			}

			if(values.size() > maxValues)
			{
				values.erase(values.begin(), values.begin() + (values.size() - maxValues));

				maxValue = std::numeric_limits<float>::min();
				minValue = std::numeric_limits<float>::max();

				for(int i = 0, end = static_cast<int>(values.size()); i < end; ++i)
				{
					if(values[i] > maxValue)
						maxValue = values[i];
					if(values[i] < minValue)
						minValue = values[i];
				}
			}
		}

		int valuesToAverage;
	};
}

struct TrackDescriptor
{
	friend class Graph;

	TrackDescriptor(float addTime, float xResolution, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f))
		: perTime(true)
		, valuesToAdd(addTime)
		, xResolution(xResolution)
		, color(color)
	{}
	TrackDescriptor(int valuesToAverage, float xResolution, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f))
		: perTime(false)
		, valuesToAdd(static_cast<float>(valuesToAverage))
		, xResolution(xResolution)
		, color(color)
	{}

private:
	bool perTime;

	float valuesToAdd;
	float xResolution;
	DirectX::XMFLOAT3 color;
};

class Graph
{
public:
	Graph();
	~Graph();

	std::string Init(ID3D11Device* device, ID3D11DeviceContext* context, ContentManager* contentManager, DirectX::XMINT2 position, DirectX::XMINT2 size, float yMax, int avgPoints, int backbufferWidth, int backbufferHeight, bool keepHistory);

	std::string AddTrack(std::string name, TrackDescriptor descriptor);
	std::string AddTracks(std::vector<std::string> trackNames, std::vector<TrackDescriptor> descriptors);

	void AddValueToTrack(const std::string& track, float value);

	void Draw(SpriteRenderer* spriteRenderer);
	void Draw();

	int GetBackgroundWidth() const;

	void Reset();
	bool DumpValues(const std::string& path) const;

private:
	int screenWidth;
	int screenHeight;
	
	int backgroundWidth;
	int legendHeight;

	int width;
	int height;

	float yMax;

	int avgPoints;

	DirectX::XMFLOAT2 position;
	std::queue<DirectX::XMFLOAT3> defaultColors;

	std::map<std::string, std::unique_ptr<Track>> tracks;

	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;

	COMUniquePtr<ID3D11Buffer> vertexBuffer;
	COMUniquePtr<ID3D11Buffer> indexBuffer;

	COMUniquePtr<ID3D11DepthStencilState> depthStencilState;
	COMUniquePtr<ID3D11BlendState> blendState;

	CharacterSet* font;
	CharacterSet* legendFont;

	int backgroundIndicies;

	VertexShader vertexShader;
	PixelShader pixelShader;

	std::vector<LegendIndex> legend;

	void CreateLegend();

	ID3D11Buffer* CreateVertexBuffer() const;
	ID3D11Buffer* CreateIndexBuffer() const;
	ID3D11Buffer* CreateBuffer(UINT size, D3D11_USAGE usage, D3D11_BIND_FLAG bindFlags, D3D11_CPU_ACCESS_FLAG cpuAccess, void* initialData /*= nullptr*/);

	int CalculateMaxPoints() const;
	int CalculateMaxValues(Track* track) const;
	float CalculateYValue(float maxValue, float value) const;
	std::string FloatToString(float value) const;
};
#endif // Graph_h__
