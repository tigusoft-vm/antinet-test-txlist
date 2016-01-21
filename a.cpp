#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <boost/any.hpp>
#include <stdexcept>

/*

TODO: move all to coding style lower_case_underscore with markers c_ t_
c_some_class::m_some_member c_foo::some_func()  t_some_type 

*/

using namespace std;

using boost::any_cast;

// nodes.at(0).AddTask(make_shared<cTask>(e_task_ask_price,'Z'));

// ==================================================================

template<typename T> T& get_front(std::vector<T> & container) {
	if (!container.size()) throw std::overflow_error("Trying to get_front of empty container");
	return container.front();
}

template<typename T> const T& get_front(const std::vector<T> & container) {
	if (!container.size()) throw std::overflow_error("Trying to get_front of empty container");
	return container.front();
}

// ==================================================================

typedef enum {
	e_task_ask_price, // what is price that you want for sending my package
	e_task_tell_price, // this is reply to above -^
} t_task;

struct cTask;
struct cTask {
	t_task mTaskKind; ///< current task
	char mName1; ///< one of paramters for the task

	cTask(t_task kind, char name1);

	void Print(ostream &out) const;
	
	bool operator==(const cTask & other) const;
};
typedef shared_ptr<cTask> tTaskPtr; ///< some pointer to task

bool cTask::operator==(const cTask & other) const {
	return (this->mTaskKind == other.mTaskKind)
	 && (this->mName1 == other.mName1);
}

void cTask::Print(ostream &out) const {
	out << "{";
	switch (mTaskKind)  {
		case e_task_ask_price: out << "ask price route-to-" << mName1; break;
		case e_task_tell_price: out << "tell price route-to" << mName1; break;
		default: out << "(invalid)";
	}
	out << "}";
}

cTask::cTask(t_task kind, char name1) 
: mTaskKind(kind), mName1(name1) { }


// ==================================================================

struct cTopic {
	vector<tTaskPtr> mTask; ///< list of my tasks
	void Print(ostream &out) const;
};
typedef shared_ptr<cTopic> tTopicPtr; ///< some pointer to other nodes

void cTopic::Print(ostream &out) const {
	out << "{ ";
	for (const tTaskPtr & objptr : mTask) { 
		objptr->Print(out);
		out << " "; // endl?
	}
	out << "}";
}

// === Message ======================================================

typedef enum {
	e_msg_task, // the type of message is simply the same as the attached task
} t_msg;

struct c_msg {
	t_msg m_kind;
	t_task m_kind_task;
	vector< boost::any > m_data;
};

// === Node =========================================================

struct cNode;
typedef shared_ptr<cNode> tNodePtr; ///< some pointer to other nodes

struct cNode : std::enable_shared_from_this<cNode> {
	char mName;

	weak_ptr<cNode> mNodePrev;
	weak_ptr<cNode> mNodeNext;

	int m_price_route;

	vector<tTopicPtr> mTopic; ///< list of my tasks

	vector< c_msg > m_inbox; ///< data that I received

	cNode(char name, int price=10);
	void Draw() const;
	void Print() const;
	void ConnectNext(shared_ptr<cNode> other);

	void Tick(); ///< Run a tick of the simulation
	void topic_tick(cTopic &topic); ///< Tick for the selected topic

	void TopicAdd(tTaskPtr task) {
		tTopicPtr topic = make_shared<cTopic>();
		mTopic.push_back(topic);
		topic->mTask.push_back(task);
	}

	void receive_msg(const c_msg & msg); ///< place it in inbox

	void react_message(const c_msg & msg); 

	cTopic& find_topic_for_task(const cTask &); ///< finds (or creates) proper Topic for such task

	bool integrate_task(shared_ptr<cTask> new_task); ///< Adds this task to proper topic, unless it's not needed. Returns 1 if was in fact added
 
	
};


// === Network ======================================================

void network_send(cNode &from, cNode &to, c_msg &msg) {
	to.receive_msg( msg );
}

// --- Node (continue) ----------------------------------------------

cNode::cNode(char name, int price) 
	: mName(name),
	mNodePrev(), mNodeNext(),
	m_price_route(price)
{ }

void cNode::Draw() const {
	ostream &out = cout;
	const bool show_arrow_detail = 0;

	auto prev = mNodePrev.lock(); 	auto next = mNodeNext.lock();

	if (prev) {
		out << "<";
		if (show_arrow_detail) out << prev->mName; 
		out << "--";
	}
	out << "["<<mName<<"]";
	if (next) {
		out << "--";
		if (show_arrow_detail) out << next->mName; 
		out << ">";
	}
}

void cNode::Print() const {
	ostream &out = cout;
	auto prev = mNodePrev.lock(); 	auto next = mNodeNext.lock();

	out << "--- " << mName << " ---" << endl;
	
	out << "Prev: ";
	if (prev) out << prev->mName; else out << "(none)";
	out << "\n";

	out << "Next: ";
	if (next) out << next->mName; else out << "(none)";
	out << "\n";


	out << "Topics: ";
	if (!mTopic.size()) out << "(none)" ; else {
		for (const auto & objptr : mTopic) { 
			out << endl << "* " ;
			objptr->Print(out);
		}
	}

	out<<"\n\n";
}



//void ConnectPrev(tNodePtr other) { mNodePrev = other; }

void cNode::ConnectNext(shared_ptr<cNode> other) { 
	mNodeNext = other; 
	other->mNodePrev = shared_from_this();
}

void cNode::Tick() { ///< Run a tick of the simulation
	// loop with possible deletion:
	for(auto it = mTopic.begin() ; it != mTopic.end() ; ) {  // for each my topic
		if (! ((*it)->mTask.size()) ) { // no tasks in this topic - must be done
	//		it = mTopic.erase( it ); // remove current
			++it;
		}
		else { // normal operation
			tTopicPtr & topic = *it; // the operator
			topic_tick( *topic ); // work on the topic
			++it; // increasing it now
		}
	}

	// read any new tasks from my inbox:
	for(auto it = m_inbox.begin() ; it != m_inbox.end() ; ) { // foreach+delete
		try {
			react_message(*it);
		}
		catch (...) { 
			cerr << "\nThere was an exception during processing inbox msg!" << endl;
		}
		it = m_inbox.erase(it);
	}
}

cTopic& cNode::find_topic_for_task(const cTask &) {
	// TODO
	if (mTopic.size() == 0) {
		cerr << " *** new topic opened *** " << (void*) this  << endl;
		cerr << mTopic.size() << endl;
		shared_ptr<cTopic> new_topic = make_shared<cTopic>();
		mTopic.push_back( new_topic ); // new topic
		cerr << mTopic.size() << endl;
	}
	cerr << "in find_topic_for_task " << mTopic.size() << endl;
	return * mTopic.at(0);
}

bool cNode::integrate_task(shared_ptr<cTask> new_task) {
	auto topic = find_topic_for_task(*new_task);
	cerr << "AFTER find_topic_for_task " << mTopic.size() << endl;

	bool contains_this_task=0;
	for(const auto & t : topic.mTask) {
		if ((*t) == (*new_task)) {  // check if has identical
			contains_this_task=1; break; 
		}
	}
	cerr << "integrate.. contains_this_task="<<contains_this_task << endl;
	if (contains_this_task) return 0; // <--- ret - has identical
	// else in fact add it:
	cerr << "tasks in topic: " << topic.mTask.size() << endl;
	topic.mTask.push_back(new_task);
	cerr << "tasks in topic: " << topic.mTask.size() << endl;
	cerr << mName << " : " << ((void*)this) << " " << topic.mTask.size() << endl;
	Print();
	return 1;
}

void cNode::react_message(const c_msg & msg) {
//	cerr << " *** react_message *** " << endl;
	switch (msg.m_kind) {
		case e_msg_task: 
		{
			switch (msg.m_kind_task) {
				case e_task_ask_price:
					auto new_task = make_shared<cTask>(
						e_task_ask_price, // it is our task to find out the price
						any_cast<char>( msg.m_data.at(0) )
					);
					integrate_task( new_task ); // add the task
				break;

				default: break;

			} // m_kind_task

//			default: ;
		} // e_msg_task - the message type is some task

		break;

		default: 
		int x=42;
		break;
	}
	Print(); cerr<<"^--- after reaction" << endl;
}

void cNode::topic_tick(cTopic &topic) { ///< Tick for the selected topic
	if (!topic.mTask.size()) return ; // nothing to do for this topic
	
	cTask & task = * get_front(topic.mTask); // the first task of it

	auto prev = mNodePrev.lock(); 	auto next = mNodeNext.lock();

	switch (task.mTaskKind)  {
		case e_task_ask_price: 
//			cerr<<"Ask price..."<<endl;
			if (task.mName1 == mName) { // I'm the goal of this question cool
				if (prev) { // we have the node who asked us
					c_msg msg;
					msg.m_kind = e_msg_task;
					msg.m_kind_task = e_task_tell_price; // we are sending REPLY
					msg.m_data.push_back( char(task.mName1) ); // data: name again
					msg.m_data.push_back( m_price_route ); // data: the price
					prev->receive_msg(msg);
				}
				else {
					cerr<<"\n*** Protocol warning: we got a request, but no prev node to reply to***\n";
				}
			}
			else { // I'm not the goal
				if (next) { // we have next node to ask
					c_msg msg;
					msg.m_kind = e_msg_task;
					msg.m_kind_task = e_task_ask_price;
					msg.m_data.push_back( char(task.mName1) ); // data with name
					next->receive_msg(msg);
				} // ask other node
				else { // no one else to ask
					cerr<<"\n*** Can not find the goal ***\n";
				}
			} // I'm not the goal
		break;

		case e_task_tell_price:
		break;
	}
}

void cNode::receive_msg( const c_msg & msg ) {
	m_inbox.push_back(msg);
}

// ==================================================================
// ==================================================================

void ClearScreen() {
	system("clear");
};

void Sleep(int ms) {
	std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
}


int main(int argc, const char** argv) {
	// options
	bool opt_run_simple = 0;
	for (int i=1; i<argc; ++i) {
		string str = argv[i];
		if (str=="--simple") opt_run_simple=1;
	}

	if (opt_run_simple) ClearScreen(); // goes well with loop.sh

	cout << "=== START ====================================" << endl;
	vector<shared_ptr<cNode>> nodes; // the world


	// build world
	nodes.push_back( make_shared<cNode>('A',10) );

	for (int ix=1; ix<6-1; ++ix) {
		nodes.push_back( make_shared<cNode>('A'+ix,10) );
	//	nodes.at(ix-1)->ConnectNext( nodes.back() );
	}
	return 0;
	nodes.push_back( make_shared<cNode>('Z',10) );
	(*(nodes.end()-1 -1))->ConnectNext( nodes.back() );


	// start tasks
	nodes.at(0)->TopicAdd(make_shared<cTask>(e_task_ask_price,'Z'));

	for (int i=0; i<3; ++i) { // main loop
		cout << endl << endl;
		cout << "=== turn # " << i << endl;

		// draw world
		for (shared_ptr<cNode> &node_shared : nodes) node_shared->Draw();
		cout<<endl;
		for (shared_ptr<cNode> &node_shared : nodes) node_shared->Print();

		if (opt_run_simple) { 
			cout << endl << "It was SIMPLE run, ending" << endl;
			return 0;
		}

		// simulate world...
		for (shared_ptr<cNode> &node_shared : nodes) node_shared->Tick();

		Sleep(400);

	} // main loop


	cout << endl << "END" << endl;

}

