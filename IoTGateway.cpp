#include <queue>
#include <mutex>
#include <condition_variable>
/*
-ThreadSafeQueue
-AlertService
-Validator
-Aggregator
-Repository
-DuplicateChecker
-DeadLetterQueue
-SensorProcessor
-IoTGateway
-main*/

template<typename T>
class ThreadSafeQueue {
	mutex mtx;
	queue<T>  q;
	int capacity ;
	std::condition_variable notfull;
	std::condition_variable notempty;
	int shutdownflag = ture
	public:
	explicit ThreadSafeQueue(int cap) : capacity(cap){}
	bool push(const T& item)
	{
		unique_lock<mutex> lock(mtx)
		notfull.wait(lock,[this)(){return q.size()< capacity || shutdownflag});
		if(shutdownflag)
			return false;
		q.push(item);
		lock.unlock();
		notempty.notify_one();
		return true;
	}

	bool pop(T&item)
	{
		unique_lock<mutex> lock(mtx);
		notempty.wait(lock,[this]{
		return !q.empty() || shutdownflag;});
		if(q.empty() || shutdownflag)
			return false;
		T item = q.front();
		q.pop();
		lock.unlock();
		notfull.notify_one();
		return true;
	}
	void shutdown()
	{
		{lock_gaurd<mutex> lock(mtx);
		shutdownflag = true;
		}
		notempty.notify_all();
		notfull.notify_all();
	}
};
struct SensorData {
    int sensorId;
    double temperature;
    long timestamp;
};

class Validator {
public:
    bool validate(const SensorData& data) {
        return data.temperature >= -50 &&
               data.temperature <= 150;
    }
};

class Aggregator {
private:
    std::mutex mtx;

    struct Stats {
        double sum = 0;
        int count = 0;
    };

    std::unordered_map<int, Stats> sensorStats;

public:
    void aggregate(const SensorData& data) {

        std::lock_guard<std::mutex> lock(mtx);

        sensorStats[data.sensorId].sum += data.temperature;
        sensorStats[data.sensorId].count++;
    }

    double getAverage(int sensorId) {

        std::lock_guard<std::mutex> lock(mtx);

        auto &s = sensorStats[sensorId];

        if(s.count == 0)
            return 0;

        return s.sum / s.count;
    }
	void printAverage()
	{	
		lock_qaurd<mutex> lock(mtx);
		cout << "Average Temperature Data from IoT Device:";
		for(auto &stat: sensorStats)
		{
			cout << std::format("Average of SensorId {} Temperature {:.2f} \n", stat.first,stat.second.sum/stat.second.count);
		}
	}
};


class AlertService {
public:

    void checkAlert(const SensorData& data) {

        if(data.temperature > 100) {
            std::cout
                << "[HIGH TEMP ALERT] Sensor="
                << data.sensorId
                << " Temp="
                << data.temperature
                << std::endl;
        }
    }
};
class Repository
{
	public:
	bool save(const SensorData & data){
	if(rand()%10 < 3)
	{
		cout << "Failed: sensor data to save for eventid << data.eventid << endl;
		return false;		
	}
	cout << "Save the sensor data into database " << data.eventid<<endl;}
};
class DuplicateChecker
{
	 mutex mtx;
	 unordered_set<long> processEvent;
	 public:
	 bool isDuplicateCheck(long eventid)
	 {
		 lock_gaurd<mutex> lock(mtx);
		 auto result = processEvent.insert(eventid);
		 return !result.second;
	 }
};
class SensorProcessor{
	Aggregator & aggregator;
	DuplicateChecker &duplicater
	Repository repository;
	Validator validater;
	AlertService alertservic;
	public:
	SensorProcessor(Aggregator &agg, DuplicateChecker &dup):aggregator(agg),duplicater(dup)
	{}
	void process(const SensorData &data)
	{
		if(!duplicater.isDuplicateCheck(data.eventid))
		{
			cout << "Duplicate Record for EventID " << data.eventid << endl;
			return;
			
		}
		if(!validater.validate(data))
		{
			cout << "Invalid Temperature for eventId : " << data.eventId << endl;
			return;
		}
		aggregator.aggregate(data);
		alertservic.checkAlert(data);
		constexpr int MAX = 3;
		for (int i = 1; i <=3; i++)
		{
			if(repository.save(data))
				return;
			cout << "Retry "<< i << " for eventid " << data.eventId << endl;
			std::this_thread::sleep_for(std::chrono::millisecond(100*i));
		}
		cout << "Max retires faild " << MAX << " Sensor Data is saved in Dead letter queue for event id" << data.eventId << endl;
		
	}
};

class IoTGateway
{
	ThreadSafeQueue<SensorData > queue;
	Aggregator aggregator;
	DuplicateChecker duplicater;
	vector<thread> workers;
	public:
	IoTGateway(int workerCount = 4):queue(10)
	{
		for (int i = 1 ; i <= workerCount; i++)
		{
			workers.push_emplace_back([this,i](){
				SensorProcessor processer(aggregator,duplicater);
				SensorData data;
				while(queue.pop(data)
				{
					processer.process(data);
					std::this_thread::sleep_for(std::chrono::millisecond(100));
				}
				cout << "Worker " << i << "Exited << endl;
		}
	}
	void receiveData(SensorData & data)
	{
		queue.push(data);
	}
	void stop()
	{
		queue.shutdown();
		for (auto t : workers)
		{
			if(t.joinable())
				t.join();
		}
	}
	void printStat()
	{
		aggregator.printAverage();
	}
};

int main() {

    IoTGateway gateway(8);

    for(int i = 1; i <= 1000; i++) {

        SensorData data;

        data.sensorId = i;
        data.temperature = rand() % 120;
        data.timestamp = time(nullptr);

        gateway.receiveData(data);
    }

    std::this_thread::sleep_for(
        std::chrono::seconds(5));

    return 0;
}


/*

                    ┌────────────────────┐
                    │      main()        │
                    │  (Producer Loop)   │
                    └─────────┬──────────┘
                              │
                              │ receiveData()
                              v
                 ┌─────────────────────────┐
                 │   ThreadSafeQueue       │
                 │   (Shared Buffer)       │
                 └─────────┬───────────────┘
                           │
          ┌────────────────┼────────────────┐
          │                │                │
          v                v                v
   ┌────────────┐  ┌────────────┐  ┌────────────┐
   │ Worker #1  │  │ Worker #2  │  │ Worker #8  │
   └─────┬──────┘  └─────┬──────┘  └─────┬──────┘
         │               │               │
         │ pop()        │ pop()        │ pop()
         v               v               v
   ┌────────────────────────────────────────────┐
   │            SensorProcessor                 │
   │                                            │
   │  1. Validator                              │
   │  2. Aggregator (shared, mutex protected)   │
   │  3. AlertService                          │
   │  4. Repository                            │
   └────────────────────────────────────────────┘
   
   SensorData
   │
   ▼
queue.push()
   │
   ▼
cv.notify_one()
   │
   ▼
Worker wakes up
   │
   ▼
queue.pop()
   │
   ▼
SensorProcessor.process()
   │
   ├── validate()
   ├── aggregate() ───► shared Aggregator (mutex)
   ├── alert check()
   └── save()
   
   
   
                   MAIN THREAD
        (generates 1000 sensor events)
                       │
                       ▼
        ┌──────────────────────────┐
        │   ThreadSafeQueue        │
        └──────────┬───────────────┘
                   │
     ┌─────────────┼────────────────────┐
     │             │                    │
     ▼             ▼                    ▼
 Worker-1      Worker-2            Worker-8
     │             │                    │
     └──────┬──────┴───────┬────────────┘
            ▼              ▼
        Processing Pipeline (parallel)
            │
            ▼
     Aggregator (shared state)
	 
	 
	         PRODUCER (main)
              │
              ▼
     THREAD-SAFE QUEUE (buffer)
              │
     ┌────────┼────────┐
     ▼        ▼        ▼
  Worker   Worker   Worker   ... (8 total)
     │        │        │
     ▼        ▼        ▼
 Processing Pipeline (validate → aggregate → alert → save)*/
