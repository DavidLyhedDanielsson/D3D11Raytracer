#ifndef Timer_h__
#define Timer_h__

#include <ctime>
#include <chrono>

class Timer
{
public:
	Timer();
	~Timer();

	void UpdateDelta();
	void ResetDelta();

	void Start();
	void Stop();
	void Reset();

	std::chrono::high_resolution_clock::duration GetTime() const;
	std::chrono::high_resolution_clock::duration GetDelta() const;

	long long GetTimeNanoseconds() const;
	long long GetTimeMicroseconds() const;
	long long GetTimeMilliseconds() const;
	float GetTimeMillisecondsFraction() const;

	bool IsRunning() const;
private:
	bool isRunning;

	std::chrono::high_resolution_clock::time_point currentTime;
	std::chrono::high_resolution_clock::time_point previousTime;
	std::chrono::high_resolution_clock::time_point creationTime;
	std::chrono::high_resolution_clock::time_point stopTime;

	std::chrono::high_resolution_clock::duration runTime;
	std::chrono::high_resolution_clock::duration pauseTime;
	std::chrono::high_resolution_clock::duration deltaTime;
};

#endif // Timer_h__
