#include "D3D11Timer.h"

#ifdef _DEBUG
#include "Logger.h"
#endif

D3D11Timer::D3D11Timer()
	: disjointQuery(nullptr, COMUniqueDeleter)
	, startTimeQuery(nullptr, COMUniqueDeleter)
	, stopTimeQuery(nullptr, COMUniqueDeleter)
{}

D3D11Timer::~D3D11Timer()
{
	for(auto& pair : queries)
	{
		if(pair.second != nullptr)
		{
			pair.second->Release();
			pair.second = nullptr;
		}
	}
}

bool D3D11Timer::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::vector<std::string> queries)
{
	this->deviceContext = deviceContext;

	D3D11_QUERY_DESC desc;
	desc.MiscFlags = 0;
	desc.Query = D3D11_QUERY_TIMESTAMP;

	ID3D11Query* queryDumb = nullptr;

	for(int i = 0, end = static_cast<int>(queries.size()); i < end; ++i)
	{
		queryDumb = nullptr;
		HRESULT hRes = device->CreateQuery(&desc, &queryDumb);
		this->queries.emplace(std::make_pair(std::move(queries[i]), queryDumb));

		if(FAILED(hRes))
			return false;
	}

	queryDumb = nullptr;
	HRESULT hRes = device->CreateQuery(&desc, &queryDumb);
	startTimeQuery.reset(queryDumb);
	if(FAILED(hRes))
		return false;

	queryDumb = nullptr;
	hRes = device->CreateQuery(&desc, &queryDumb);
	stopTimeQuery.reset(queryDumb);
	if(FAILED(hRes))
		return false;

	desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	queryDumb = nullptr;
	hRes = device->CreateQuery(&desc, &queryDumb);
	disjointQuery.reset(queryDumb);
	if(FAILED(hRes))
		return false;

	return true;
}

void D3D11Timer::Start()
{
	deviceContext->Begin(disjointQuery.get());
	deviceContext->End(startTimeQuery.get());
}

void D3D11Timer::Stop(const std::string& string)
{
#ifdef _DEBUG
	if(queries.count(string) == 0)
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Tried to stop non-existent query \"" + string + "\"");
		return;
	}
#endif

	deviceContext->End(queries[string]);
}

std::map<std::string, double> D3D11Timer::Stop()
{
	deviceContext->End(stopTimeQuery.get());
	deviceContext->End(disjointQuery.get());

	UINT64 startTime = 0;
	UINT64 stopTime = 0;

	while(deviceContext->GetData(startTimeQuery.get(), &startTime, sizeof(startTime), 0) != S_OK);
	while(deviceContext->GetData(stopTimeQuery.get(), &stopTime, sizeof(stopTime), 0) != S_OK);

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
	while(deviceContext->GetData(disjointQuery.get(), &disjointData, sizeof(disjointData), 0) != S_OK);

	std::map<std::string, double> timeTable;
	if(disjointData.Disjoint == FALSE)
	{
		double frequency = static_cast<double>(disjointData.Frequency);

		for(const auto& query : queries)
		{
			UINT64 queryTime = 0;
			while(deviceContext->GetData(query.second, &queryTime, sizeof(queryTime), 0) != S_OK);

			UINT64 queryDelta = queryTime - startTime;

			timeTable.emplace(std::make_pair(query.first, (queryDelta / frequency) * 1000.0));
		}
	}

	return timeTable;
}
