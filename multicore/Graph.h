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
}

class Track
{
public:
	enum class AVERAGE_TYPE { PER_SECOND, PER_ADD };

	Track()
		: averageType(AVERAGE_TYPE::PER_ADD)
		, color(-1.0f, -1.0f, -1.0f)
		, maxValues(0)
		, valuesToAverage(1)
		, xResolution(1.0f)
		, maxValue(std::numeric_limits<float>::min())
		, minValue(std::numeric_limits<float>::max())
		, addedValues(0)
	{
		values.emplace_back(0.0f);
	}
	Track(int valuesToAverage, float xResolution, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f))
		: averageType(AVERAGE_TYPE::PER_ADD)
		, valuesToAverage(static_cast<float>(valuesToAverage))
		, xResolution(xResolution)
		, color(color)
		, maxValue(std::numeric_limits<float>::min())
		, minValue(std::numeric_limits<float>::max())
		, addedValues(0)
	{
		values.emplace_back(0.0f);
	}
	Track(float valuesToAverage, float xResolution, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f))
		: averageType(AVERAGE_TYPE::PER_SECOND)
		, valuesToAverage(valuesToAverage)
		, xResolution(xResolution)
		, color(color)
		, maxValue(std::numeric_limits<float>::min())
		, minValue(std::numeric_limits<float>::max())
		, addedValues(0)
	{
		values.emplace_back(0.0f);

		timer.Start();
	}

	void AddValue(float value)
	{
		if(averageType == AVERAGE_TYPE::PER_SECOND)
		{
			if(timer.GetTimeMillisecondsFraction() * 0.001f >= valuesToAverage)
			{
				if(values.back() < minValue)
					minValue = values.back();

				values.emplace_back(0.0f);
				timer.Reset();
			}

		}
		else
		{
			if(addedValues == static_cast<int>(valuesToAverage))
			{
				if(values.back() < minValue)
					minValue = values.back();

				addedValues = 0;
				values.emplace_back(0.0f);
			}

		}

		++addedValues;
		values.back() += value;

		if(values.back() > maxValue)
			maxValue = values.back();

		if(values.size() > maxValues)
		{
			values.erase(values.begin(), values.begin() + (values.size() - maxValues));

			maxValue = std::numeric_limits<float>::min();
			minValue = std::numeric_limits<float>::max();

			//for(float value : values)
			for(int i = 0, end = static_cast<int>(values.size()); i < end; ++i)
			{
				if(values[i] > maxValue)
					maxValue = values[i];
				if(values[i] < minValue && i < end - 1)
					minValue = values[i];
			}
		}
	}

	float GetValue(int index) const
	{
		return values[index];
	}

	float GetMaxValue() const
	{
		return maxValue / static_cast<float>(valuesToAverage);
	}

	float GetMinValue() const
	{
		return minValue / static_cast<float>(valuesToAverage);
	}

	std::deque<float> GetValues() const
	{
		return values;
	}

	void Clear()
	{
		values.clear();
		addedValues = 0;
		maxValue = 0;
		minValue = 0;
	}

	AVERAGE_TYPE averageType;

	DirectX::XMFLOAT3 color;
	std::deque<float> values;

	int maxValues;
	float valuesToAverage;
	int addedValues;
	float xResolution;
	float maxValue;
	float minValue;

	Timer timer;
};

class Graph
{
public:
	Graph();
	~Graph();

	std::string Init(ID3D11Device* device, ID3D11DeviceContext* context, ContentManager* contentManager, DirectX::XMINT2 position, DirectX::XMINT2 size, float yMax, int avgPoints, int backbufferWidth, int backbufferHeight, bool keepHistory);

	std::string AddTrack(std::string name, Track track);
	std::string AddTracks(std::vector<std::string> trackNames, std::vector<Track> tracks);

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

	std::map<std::string, Track> tracks;

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
	int CalculateMaxValues(const Track& track) const;
	float CalculateYValue(float maxValue, float value) const;
	std::string FloatToString(float value) const;
};
#endif // Graph_h__
