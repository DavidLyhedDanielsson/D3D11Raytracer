#ifndef D3D11Timer_h__
#define D3D11Timer_h__

#include <map>
#include <string>
#include <vector>

#include "Common.h"

class D3D11Timer
{
public:
	D3D11Timer();
	~D3D11Timer();

	void operator=(D3D11Timer&) = delete;
	D3D11Timer(D3D11Timer&) = delete;

	bool Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::vector<std::string> queries);

	void Start();
	void Stop(const std::string& string);
	std::map<std::string, double> Stop();

private:
	COMUniquePtr<ID3D11Query> disjointQuery;
	COMUniquePtr<ID3D11Query> startTimeQuery;
	COMUniquePtr<ID3D11Query> stopTimeQuery;

	std::map<std::string, ID3D11Query*> queries;
	std::vector<std::string> stopOrder;

	ID3D11DeviceContext* deviceContext;
};

#endif // D3D11Timer_h__
