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
	e_task_ask_price,
	e_task_tell_price,
} t_task;

struct cTask;
struct cTask {
	t_task mTaskKind; ///< current task
	char mName1; ///< one of paramters for the task

	cTask(t_task kind, char name1);

	void Print(ostream &out) const;
};
typedef shared_ptr<cTask> tTaskPtr; ///< some pointer to task

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

	tNodePtr mNodePrev;
	tNodePtr mNodeNext;

	int m_price_route;

	vector<tTopicPtr> mTopic; ///< list of my tasks

	vector< c_msg > m_inbox; ///< data that I received

	cNode(char name, int price=10);
	void Draw() const;
	void Print() const;
	void ConnectNext(tNodePtr other);

	void Tick(); ///< Run a tick of the simulation
	void topic_tick(cTopic &topic); ///< Tick for the selected topic

	void TopicAdd(tTaskPtr task) {
		tTopicPtr topic = make_shared<cTopic>();
		mTopic.push_back(topic);
		topic->mTask.push_back(task);
	}

	void receive_msg(const c_msg & msg); ///< place it in inbox

	void react_message(const c_msg & msg); 
 
	
};


// === Network ======================================================

void network_send(cNode &from, cNode &to, c_msg &msg) {
	to.receive_msg( msg );
}

// --- Node (continue) ----------------------------------------------

cNode::cNode(char name, int price) 
	: mName(name),
	mNodePrev(nullptr), mNodeNext(nullptr),
	m_price_route(price)
{ }

void cNode::Draw() const {
	ostream &out = cout;
	const bool show_arrow_detail = 0;

	if (mNodePrev) {
		out << "<";
		if (show_arrow_detail) out << mNodePrev->mName; 
		out << "--";
	}
	out << "["<<mName<<"]";
	if (mNodeNext) {
		out << "--";
		if (show_arrow_detail) out << mNodeNext->mName; 
		out << ">";
	}
}

void cNode::Print() const {
	ostream &out = cout;

	out << "--- " << mName << " ---" << endl;
	
	out << "Prev: ";
	if (mNodePrev) out << mNodePrev->mName; else out << "(none)";
	out << "\n";

	out << "Next: ";
	if (mNodeNext) out << mNodeNext->mName; else out << "(none)";
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

void cNode::ConnectNext(tNodePtr other) { 
	mNodeNext = other; 
	other->mNodePrev = shared_from_this();
}

void cNode::Tick() { ///< Run a tick of the simulation
	// loop with possible deletion:
	for(auto it = mTopic.begin() ; it != mTopic.end() ; ) {  // for each my topic
		if (! ((*it)->mTask.size()) ) { // no tasks in this topic - must be done
			it = mTopic.erase( it ); // remove current
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

void cNode::react_message(const c_msg & msg) {
	switch (msg.m_kind) {
		case e_msg_task: 
		{
			switch (msg.m_kind_task) {
				case e_task_ask_price:
				break;
			} // m_kind_task
		} // e_msg_task - the message type is some task

		break;
	}
}

void cNode::topic_tick(cTopic &topic) { ///< Tick for the selected topic
	if (!topic.mTask.size()) return ; // nothing to do for this topic
	
	cTask & task = * get_front(topic.mTask); // the first task of it

	switch (task.mTaskKind)  {
		case e_task_ask_price: 
			cerr<<"Ask price..."<<endl;
			if (task.mName1 == mName) { // I'm the goal of this question cool
				if (mNodePrev) { // we have the node who asked us
					c_msg msg;
					msg.m_kind = e_msg_task;
					msg.m_kind_task = e_task_tell_price; // we are sending REPLY
					msg.m_data.push_back( char(task.mName1) ); // data: name again
					msg.m_data.push_back( m_price_route ); // data: the price
					mNodePrev->receive_msg(msg);
				}
				else {
					cerr<<"\n*** Protocol warning: we got a request, but no prev node to reply to***\n";
				}
			}
			else { // I'm not the goal
				if (mNodeNext) { // we have next node to ask
					c_msg msg;
					msg.m_kind = e_msg_task;
					msg.m_kind_task = e_task_ask_price;
					msg.m_data.push_back( char(task.mName1) ); // data with name
					mNodeNext->receive_msg(msg);
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
		nodes.at(ix-1)->ConnectNext( nodes.back() );
	}
	nodes.push_back( make_shared<cNode>('Z',10) );
	(*(nodes.end()-1 -1))->ConnectNext( nodes.back() );

	// start tasks
	nodes.at(0)->TopicAdd(make_shared<cTask>(e_task_ask_price,'Z'));

	for (int i=0; i<10; ++i) { // main loop
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

