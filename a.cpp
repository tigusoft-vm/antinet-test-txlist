#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <thread>
#include <boost/any.hpp>
#include <stdexcept>
#include <cassert>

/*

A -> B -> C -> D -> Z

send packet
  (get_price)
  ask_price  ----> offer_price
    todo             (get next hop price)
    wait             ask_price
                       todo -----------------> offer_price
                       wait                    my_price
                       done <-----------------
                     tell_price
                        50+10
    done <------------ todo

e_task_ask_price   ( "dst"="Z" , ["buyer"="B" === msg.from] , ["seller"="C" === msg.to  ] )
e_task_tell_price  ( "dst"="Z" , ["buyer"="B" === msg.to  ] , ["seller"="C" === msg.from] )

*/

using namespace std;
using boost::any_cast;

#if 0
// a quick test
struct foo {

int main() {
}

#else


// nodes.at(0).AddTask(make_shared<cTask>(e_task_ask_price,'Z'));

// ==================================================================

template<typename T> typename T::value_type & get_front(T & container) {
	if (!container.size()) throw std::overflow_error("Trying to get_front of empty container");
	return container.front();
}

template<typename T> typename T::iterator get_begin_notempty(T & container) {
	if (!container.size()) throw std::overflow_error("Trying to get begin of empty container");
	auto it = container.begin();
	assert(it != container.end());
	return it;
}

// ==================================================================

typedef enum {
	e_task_ask_price, // what is price that you want for sending my package
	e_task_tell_price, // this is reply to above -^
} t_task;

typedef enum {
	e_state_done=0, ///< this task is done
	e_state_todo=1, ///< this task is pending [the usual/first step of] execution

	// for e_task_ask_price:
	// todo - task will be sent
	// todo
	// done

} t_state;

struct c_data_tag {
	public:
		std::string m_name;

		c_data_tag() : m_name("x") { }
		c_data_tag(const std::string &name) : m_name(name) { }
		c_data_tag(const char* s) : m_name(s) { }

		c_data_tag(const c_data_tag & org) : m_name(org.m_name) { }
		c_data_tag(c_data_tag && org) : m_name(std::move(org.m_name)) { }  //  { m_name = std::move(org.m_name)    or (almost the same)   std::swap(m_name, org.m_name)  // XXX

		bool operator<(const c_data_tag & other) { return (*this).m_name < other.m_name; }
		bool operator==(const c_data_tag & other) { return (*this).m_name == other.m_name; }

		// operator string() const { return m_name; }
};

ostream& operator<<(ostream& oss, const c_data_tag & obj) {
	oss << obj.m_name;
	return oss;
}


/***
 * @brief This is container of data elements that are indexed by name (e.g. key is: c_data_tag keys), 
 * and the values are casted/converted on read. Any data can be written, as in boost::any
 */
template <typename TKey>
class map_of_any {
	private:
		typedef map< TKey , boost::any > t_container; ///< type of the data 
		t_container m_container; ///< the data

	public:
	
		bool operator==( const map_of_any<TKey> & other) const {
			return m_container == other.m_container;
		}
		bool operator!=( const map_of_any<TKey> & other) const {
			return ! ( *this == other );
		}
		
	  /***
	   * @brief This reads the data, the data must already exist and must be of the specified type (as with boost::any_cast)
	   */
		template<typename T> const T & read_data(const TKey & tag) const { 
			try {
				const boost::any * any_data = & m_container.at(tag);
				cout << __LINE__ << " " << any_cast<T>(& m_container.at(tag)) << endl;
				try {
					return * any_cast<T>(any_data);
				}
				catch(boost::bad_any_cast) { cerr << "Can not use data from tag=["<<tag<<"]" << " as " << typeid(T).name() << " because it has type: " << any_data->type().name() << endl; throw ; }
			}
			catch(std::out_of_range) { cerr << "Can not use data from tag=["<<tag<<"]" << " as " << typeid(T).name() << " because the key does not exist." << endl; throw ; }
		}

	  /***
	   * @brief This writes (sets) the data, it must be same type as before unless allow_change_type==true
	   * @return Says if it was an update of already existing entry (true) - else it was a creation of new object
	   */
		template<typename T> bool write_data(const TKey & tag, T && data, bool allow_change_type=false) { // opti-todo(&&)
			auto result = m_container.emplace(tag, std::forward<T>(data));
			if (result.second) return false; // new element inserted
			// else - already exists, need to overwrite value:
			const auto found = result.first;
			boost::any & found_any = found->second;
			if (found_any.type() == typeid(data)) { // same type
				found_any = data; // replace the data in found element
			} else { // other type then previously
				if (allow_change_type) { found_any = data; } // overwrite and change type
				else throw boost::bad_any_cast(); // "Tried to change the type of object by overwritting it, while it was not allowed.");
			}
			return true; // updated
		}
};

typedef map_of_any<c_data_tag> c_data_named; ///< map of named (e.g. string-like) variables (of any any).

using namespace std;

/*
typedef string dom;

struct miasto { 
	dom a,b;

	dom front() { return a; } // a - lv
	dom forme() { return a+"ale inny"; } // RVO   expression is rvalue

	const dom & front2() { return a; } // lv
};

int main() {
	{
		miasto krk;

		dom best = krk.front();

		const & best = krk.front();
		krk.przebuduj();
		best.draw(); // !!!


		krk.front().draw();

		grafika.draw( krk.front() ); // ?
		grafika.draw( krk , krk.front() ); // ?

	}
}
*/

int main() { 
	{
		std::unique_ptr<map_of_any<string>> map( new map_of_any<string>() );
		int a=50;
		const int b=100;
		map->write_data("foo", 42);
		map->write_data("foo", 42+10);
		map->write_data("bar", a);
		map->write_data("bar", b);
		std::cout << map->read_data<int>("foo") << std::endl;



		const int & foo = map->read_data<int>("foo"); 
		cout << __LINE__ << " got it as " << & foo << endl;
		// ^-- if would be returned by a value, then this would be a tricky way to accept the result, do NOT do it (std 8.3.5: lifetime of temporary value will be prolonged when stored into local const)

		std::cout << foo << std::endl;
		int foo2 = map->read_data<int>("bar");
		std::cout << foo << " " << foo2 << std::endl;
		//= map.reset(nullptr);
		//tab.at(0); // map->read_data<int>("foo");
//		tab.push_back()...
//		foo ....



	}
}

#if 0

class c_task;
class c_task {
	public:
		t_task m_task_kind; ///< current task
		t_state m_state; ///<  state of this task

		c_data_named m_data; ///< the data, accessible by tags

		c_task(t_task kind);

		void print(ostream &out) const;

		bool operator==(const c_task & other) const;
		bool is_done() const { return m_state == e_state_done; }

};
typedef shared_ptr<c_task> tTaskPtr; ///< some pointer to task

bool c_task::operator==(const c_task & other) const {
	if (! (this->m_task_kind == other.m_task_kind)) return false;
	if (this->m_data != other.m_data) return false;
	return true;
}

void c_task::print(ostream &out) const {
	out << "{";
	switch (m_task_kind)  {
		case e_task_ask_price: out << "ask price route-to-" << m_data.read_data<string>("dst") ; break;
		case e_task_tell_price: out << "tell price route-to" << m_data.read_data<string>("dst") ; break;
		default: out << "(invalid)";
	}
	out << "}";
}

c_task::c_task(t_task kind)
: m_task_kind(kind), m_state(e_state_todo) { }


// ==================================================================

struct c_topic {
	vector<tTaskPtr> mTask; ///< list of my tasks
	void print(ostream &out) const;
};
typedef shared_ptr<c_topic> tTopicPtr; ///< some pointer to other nodes

ostream& operator<<(ostream& oss, const c_topic & obj) {
	obj.print(oss);
	return oss;
}

void c_topic::print(ostream &out) const {
	out << " TOPIC (at"<<(void*)this<<") with " << mTask.size()<<" tasks: { ";
	for (const tTaskPtr & objptr : mTask) {
		objptr->print(out);
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
	c_data_named m_data; ///< data of this request, often includes the task
};

// === Node =========================================================

struct c_node;
typedef shared_ptr<c_node> tNodePtr; ///< some pointer to other nodes

struct c_node : std::enable_shared_from_this<c_node> {
	typedef string t_name;
	t_name m_name; ///< the name of the node (e.g. ID of node)

	weak_ptr<c_node> m_node_prev;
	weak_ptr<c_node> m_node_next;

	int m_price_route;

	vector<tTopicPtr> mTopic; ///< list of my tasks

	vector< c_msg > m_inbox; ///< data that I received

	c_node(char name, int price=10);
	void draw() const;
	void print() const;
	void connect_next(shared_ptr<c_node> other);

	void tick(); ///< Run a tick of the simulation
	void topic_tick(c_topic &topic); ///< tick for the selected topic
	c_task* topic_get_first_task_clear(c_topic &topic); ///< get the first task of topic (or NULL) but first remove all done tasks

	void topic_add(tTaskPtr task) {
		tTopicPtr topic = make_shared<c_topic>();
		mTopic.push_back(topic);
		topic->mTask.push_back(task);
	}

	void receive_msg(const c_msg & msg); ///< place it in inbox

	void react_message(const c_msg & msg);

	c_topic& find_topic_for_task(const c_task &); ///< finds (or creates) proper Topic for such task

	bool integrate_task(shared_ptr<c_task> new_task); ///< Adds this task to proper topic, unless it's not needed. Returns 1 if was in fact added

	void print(std::ostream & oss) const;
};

ostream& operator<<(ostream& oss, const c_node & obj) {
	obj.print(oss);
	return oss;
}

void c_node::print(std::ostream & oss) const {
	oss << "(NODE at "<<this<<") " << m_name ;
}


// === Network ======================================================

void network_send(c_node &from, c_node &to, c_msg &msg) {
	to.receive_msg( msg );
}

// --- Node (continue) ----------------------------------------------

c_node::c_node(char name, int price)
	: m_name( string("") + name),
	m_node_prev(), m_node_next(),
	m_price_route(price)
{ }

void c_node::draw() const {
	ostream &out = cout;
	const bool show_arrow_detail = 0;

	auto prev = m_node_prev.lock(); 	auto next = m_node_next.lock();

	if (prev) {
		out << "<";
		if (show_arrow_detail) out << prev->m_name;
		out << "--";
	}
	out << "["<<m_name<<"]";
	if (next) {
		out << "--";
		if (show_arrow_detail) out << next->m_name;
		out << ">";
	}
}

void c_node::print() const {
	ostream &out = cout;
	auto prev = m_node_prev.lock(); 	auto next = m_node_next.lock();

	out << "--- " << m_name << " ---" << endl;

	out << "Prev: ";
	if (prev) out << prev->m_name; else out << "(none)";
	out << " / ";

	out << "Next: ";
	if (next) out << next->m_name; else out << "(none)";
	out << "\n";

	out << "Topics: ";
	if (!mTopic.size()) out << "(none)" ; else {
		for (const auto & objptr : mTopic) {
			out << endl << "* " ;
			objptr->print(out);
		}
	}

	out<<"\n\n";
}



//void ConnectPrev(tNodePtr other) { m_node_prev = other; }

void c_node::connect_next(shared_ptr<c_node> other) {
	m_node_next = other;
	other->m_node_prev = shared_from_this();
}

void c_node::tick() { ///< Run a tick of the simulation
	// loop with possible deletion:
	for(auto it = mTopic.begin() ; it != mTopic.end() ; ) {  // for each my topic
		if (! ((*it)->mTask.size()) ) { // no tasks in this topic - must be done
			it = mTopic.erase( it ); // remove current
		}
		else { // topic has something to do
			tTopicPtr & topic = *it; // the operator
			topic_tick( *topic ); // work on the topic
			++it; // increasing it now
		}
	}

	// read any new tasks from my inbox - and react
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

c_topic& c_node::find_topic_for_task(const c_task &) {
	// TODO more advanced
	if (mTopic.size() == 0) {
		shared_ptr<c_topic> new_topic = make_shared<c_topic>();
		mTopic.push_back( new_topic ); // new topic
	}
	shared_ptr<c_topic> new_topic = make_shared<c_topic>();
	mTopic.push_back( new_topic );
	return * mTopic.back(); // return the just added topic; MEM:our object owns it (pins it)
}

bool c_node::integrate_task(shared_ptr<c_task> new_task) {
	auto & topic = find_topic_for_task(*new_task);
	bool contains_this_task=0;
	for(const auto & t : topic.mTask) {
		if ((*t) == (*new_task)) {  // check if has identical
			contains_this_task=1; break;
		}
	}
	if (contains_this_task) return 0; // <--- ret - has identical
	// else in fact add it:
	topic.mTask.push_back(new_task);
	return 1; // no problem
}

void c_node::react_message(const c_msg & msg) {
	switch (msg.m_kind) {
		case e_msg_task: // a message to create task
		{
			const	c_task & = msg.get_data<c_task>("task");

		 	switch (msg.m_kind_task) {

				case e_task_ask_price: {
					auto new_task = make_shared<c_task>(
						e_task_ask_price, // it is our task to find out the price
						any_cast<char>( msg.m_data.at(0) )
					);
					integrate_task( new_task ); // add the task
				}	break;

				default: { } break; // other values of msg.m_kind_task
			} // switch msg.m_kind_task

		} break; // e_msg_task - the message type is some task
		break;

		default: { } break; // other values of msg.m_kind
	} // switch msg.m_kind
	// print(); cerr<<"^--- after reaction" << endl;
}

c_task* c_node::topic_get_first_task_clear(c_topic &topic) {
	while (true) {
		if (!topic.mTask.size()) return nullptr ; // <--- nothing to do for this topic it is empty (now)

		auto task_it = get_begin_notempty( topic.mTask ); // the first task of it - iterator
		if ( (*task_it)->is_done()) { // task is done
			topic.mTask.erase(task_it); // erase it
		} else {
			return (*task_it).get(); // <-- return the task
		}
		// continue (e.g. after removing, try next task)
	}
}

void c_node::topic_tick(c_topic &topic) { ///< tick for the selected topic
	auto * task_rawptr = topic_get_first_task_clear(topic);
	if (! task_rawptr) return ; // <--- this topic is empty after all

	auto & task = *task_rawptr; // the first task object
	auto prev = m_node_prev.lock(); 	auto next = m_node_next.lock();

	switch (task.m_task_kind)  {

		case e_task_ask_price: { // the request is to find the price
			auto & goal_name = task.m_data.read_data<string>("dst");
			cerr<<"\n*** asking price, goal_name="<<goal_name<<endl;
			if (goal_name == m_name) { // I'm the goal of this question! cool
				/*
				if (prev) { // we have the node who asked us so we can reply to them
					cerr<<"\n*** replied to prev="<<(*prev)<<endl;
					c_msg msg;
					msg.m_kind = e_msg_task;
					msg.m_kind_task = e_task_tell_price; // we are sending REPLY
					msg.m_data.push_back( char(task.m_name1) ); // data: name again
					msg.m_data.push_back( m_price_route ); // data: the price
					prev->receive_msg(msg);
				}
				else {
					cerr<<"\n*** Protocol warning: we got a request, but no prev node to reply to***\n";
				}
				*/
				auto new_task = make_shared<c_task>( e_task_tell_price, task.m_data.read_data<string>("dst") );
				integrate_task( new_task ); // add the task

			}
			else { // I'm not the goal
				if (next) { // we have next node to ask
					cerr<<"\nforwardnig to next="<<(*next)<<endl;
					c_msg msg;
					msg.m_kind = e_msg_task;
					msg.m_kind_task = e_task_ask_price;
					msg.m_data.push_back( char(task.m_data_extra.at("dst")) ); // data with name
					next->receive_msg(msg); // <--- ask them
					task.m_state = e_state_done;
				} // ask other node
				else { // no one else to ask
					cerr<<"\n*** Can not find the goal, in " << m_name << " " << (void*)this
						<< "the goal is: " << goal_name << endl;
				}
			} // I'm not the goal
		} break; // task e_task_ask_price

		case e_task_tell_price: {
			cerr<<"\n *** *** tell the price..." << endl;
		} break;
	}
}

void c_node::receive_msg( const c_msg & msg ) {
	m_inbox.push_back(msg);
}

// ==================================================================
// ==================================================================

void clear_screen() {
	system("clear");
};

void sleep_seconds(double ms) {
	cout<<std::flush;
	cerr<<std::flush;
	std::this_thread::sleep_for( std::chrono::milliseconds( (long int)(ms*1000)  ) );
}


int main(int argc, const char** argv) {
	// options
	bool opt_run_simple = 0;
	for (int i=1; i<argc; ++i) {
		string str = argv[i];
		if (str=="--simple") opt_run_simple=1;
	}

	if (opt_run_simple) clear_screen(); // goes well with loop.sh

	int the_delay = -1;  // the delay, in ms, or -1 to wait for ENTER

	cout << "=== START ====================================" << endl;
	vector<shared_ptr<c_node>> nodes; // the world


	// build world
	nodes.push_back( make_shared<c_node>('A',10) );

	for (int ix=1; ix<6-1; ++ix) {
		nodes.push_back( make_shared<c_node>('A'+ix,10) );
		nodes.at(ix-1)->connect_next( nodes.back() );
	}
	nodes.push_back( make_shared<c_node>('Z',10) );
	(*(nodes.end()-1 -1))->connect_next( nodes.back() );


	// start tasks
	nodes.at(0)->topic_add(make_shared<c_task>(e_task_ask_price,'Z'));

	for (int i=0; i<100; ++i) { // main loop
		cout << endl << endl;
		cout << "=== turn # " << i << endl;

		// draw world
		for (shared_ptr<c_node> &node_shared : nodes) node_shared->draw();
		cout<<endl;
		for (shared_ptr<c_node> &node_shared : nodes) node_shared->print();

		if (opt_run_simple) {
			cout << endl << "It was SIMPLE run, ending" << endl;
			return 0;
		}

		// simulate world...
		for (shared_ptr<c_node> &node_shared : nodes) node_shared->tick();

		if (the_delay==0) ;
		else if (the_delay==-1) { cout<<"Press ENTER"<<endl; string nothing; getline(cin, nothing); }
		else if (the_delay>0) sleep_seconds(the_delay / 1000.);
		else assert(0);
	} // main loop


	cout << endl << "END" << endl;

}

#endif

#endif

