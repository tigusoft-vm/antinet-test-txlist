#include <iostream>
#include <vector>
#include <memory>

using namespace std;

// nodes.at(0).AddTask(make_shared<cTask>(e_task_ask_price,'Z'));

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
		case e_task_ask_price: out << " ask price route-to-" << mName1; break;
		case e_task_tell_price: out << " tell price route-to" << mName1; break;
		default: out << "(invalid)";
	}
	out << "}";
}

cTask::cTask(t_task kind, char name1) 
: mTaskKind(kind), mName1(name1) { }


struct cTopic {
	vector<tTaskPtr> mTask; ///< list of my tasks
	void Print(ostream &out) const;
};
typedef shared_ptr<cTopic> tTopicPtr; ///< some pointer to other nodes

void cTopic::Print(ostream &out) const {
	out << "topic {";
	for (const tTaskPtr & objptr : mTask) { 
		objptr->Print(out);
		out << endl;
	}
	out << "}" << endl;
}


struct cNode;
typedef shared_ptr<cNode> tNodePtr; ///< some pointer to other nodes
struct cNode : std::enable_shared_from_this<cNode> {
	char mName;

	tNodePtr mNodePrev;
	tNodePtr mNodeNext;

	int m_price_route;

	vector<tTopicPtr> mTopic; ///< list of my tasks

	cNode(char name, int price=10);
	void Draw() const;
	void Print() const;
	void ConnectNext(tNodePtr other);

	void TopicAdd(tTaskPtr task) {
		tTopicPtr topic = make_shared<cTopic>();
		mTopic.push_back(topic);
		topic->mTask.push_back(task);
	}
};

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
	for (const auto & objptr : mTopic) { 
		objptr->Print(out);
		out << endl;
	}
	

	out<<"\n\n";
}

//void ConnectPrev(tNodePtr other) { mNodePrev = other; }

void cNode::ConnectNext(tNodePtr other) { 
	mNodeNext = other; 
	other->mNodePrev = shared_from_this();
}



int main(int argc, const char** argv) {
	// options
	bool opt_run_simple = 0;
	for (int i=1; i<argc; ++i) {
		string str = argv[i];
		if (str=="--simple") opt_run_simple=1;
	}

	if (opt_run_simple) system("clear"); // goes well with loop.sh

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

	// draw world
	for (shared_ptr<cNode> &node_shared : nodes) node_shared->Draw();
	cout<<endl;
	for (shared_ptr<cNode> &node_shared : nodes) node_shared->Print();

	if (opt_run_simple) { 
		cout << endl << "It was SIMPLE run, ending" << endl;
		return 0;
	}

	// simulate world...


	cout << endl << "END" << endl;

}

