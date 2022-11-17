#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <assert.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

static uint64_t
timer_nsec()
{
#if defined(CLOCK_MONOTONIC_RAW)
	const clockid_t clockid = CLOCK_MONOTONIC_RAW;
#else
	const clockid_t clockid = CLOCK_MONOTONIC;
#endif

	timespec t;
	clock_gettime(clockid, &t);

	return t.tv_sec * 1000000000ULL + t.tv_nsec;
}

enum {
	BARRIER_START,
	BARRIER_FINISH,
	BARRIER_COUNT
};

static pthread_barrier_t barrier[BARRIER_COUNT];

template < typename ITEM_T >
class pool_alloc
{
public:
	typedef size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef ITEM_T* pointer;
	typedef const ITEM_T* const_pointer;
	typedef ITEM_T& reference;
	typedef const ITEM_T& const_reference;
	typedef ITEM_T value_type;

private:
	pointer const pool_base;
	size_type const pool_size;
	size_type occupancy;

	pool_alloc() throw() :
	pool_base(0), pool_size(0), occupancy(0)
	{}

	template < typename OTHER_T >
	pool_alloc(pool_alloc< OTHER_T > const&) throw()
	{}

public:
	template < typename OTHER_T >
	struct rebind
	{ typedef pool_alloc< OTHER_T > other; };

	pool_alloc(
		const pool_alloc& src) throw() :
		pool_base(src.pool_base),
		pool_size(src.pool_size),
		occupancy(src.occupancy)
	{}

	pool_alloc(
		const pointer base,
		const size_type size) :
		pool_base(base),
		pool_size(size),
		occupancy(0)
	{}

	~pool_alloc() throw()
	{}

	pointer
	address(reference x) const
	{
		return std::__addressof(x);
	}

	const_pointer
	address(const_reference x) const
	{
		return std::__addressof(x);
	}

	pointer
	allocate(
		size_type n,
		const void* = 0)
	{
		assert(pool_size >= occupancy + n);

		const size_type o = occupancy;
		occupancy += n;

		return pool_base + o;
	}

	void
	deallocate(pointer p, size_type n)
	{
		assert(p == pool_base);
		assert(n == occupancy);

		occupancy = 0;
	}

	size_type
	max_size() const throw() 
	{
		return pool_size;
	}

	void 
	construct(pointer p, const ITEM_T& val) 
	{
		::new ((void *)p) value_type(val);
	}

	void 
	destroy(pointer p)
	{
		p->~ITEM_T();
	}
};

template< typename ITEM_T >
inline bool
operator ==(
	const pool_alloc< ITEM_T >&,
	const pool_alloc< ITEM_T >&)
{
	return true;
}

template< typename ITEM_T >
inline bool
operator !=(
	const pool_alloc< ITEM_T >&,
	const pool_alloc< ITEM_T >&)
{
	return false;
}


typedef std::pair< int, int > Node; // parent index, payload
typedef std::vector< Node, pool_alloc< Node > > Tree;
typedef Tree::difference_type NodeIdx;
typedef std::vector< NodeIdx, pool_alloc< NodeIdx > > Translator;

static const Node tree[] =
{
	Node( 1, 0),
	Node( 6, 1),
	Node( 1, 2),
	Node( 6, 3),
	Node( 3, 4),
	Node( 3, 5),
	Node(-1, 6)
};

static const size_t reps = unsigned(1e+7) * 6;
static const size_t nthreads = TREE_NUM_THREADS;
static const size_t one_less = nthreads - 1;
static const size_t tree_size = sizeof(tree) / sizeof(tree[0]);
static const size_t pool_size = tree_size * nthreads;

Node na[pool_size] __attribute__ ((aligned (16)));
NodeIdx ta[pool_size] __attribute__ ((aligned (16)));


template < typename TREE_CONST_ITER_T >
static TREE_CONST_ITER_T
find_first_child(
	const TREE_CONST_ITER_T& begin,
	const TREE_CONST_ITER_T& end,
	const NodeIdx parent_idx)
{
	TREE_CONST_ITER_T it = begin;

	while (it != end)
	{
		if (parent_idx == it->first)
			return it;
		++it;
	}

	return end;
}


template < typename TREE_CONST_ITER_T >
static void
add_node(
	const TREE_CONST_ITER_T& tree_src_begin,
	Tree& tree_dst,
	const NodeIdx node_idx,
	const NodeIdx new_parent_idx,
	Translator& trans)
{
	assert(tree_dst.size() == trans.size());

	tree_dst.push_back(Node(new_parent_idx, (tree_src_begin + node_idx)->second));
	trans.push_back(node_idx);
}


template < typename TREE_CONST_ITER_T >
static void
bfs_tree(
	const TREE_CONST_ITER_T& tree_src_begin,
	const TREE_CONST_ITER_T& tree_src_end,
	Tree& tree_dst,
	Translator& trans)
{
	assert(trans.empty());

	const TREE_CONST_ITER_T root_it = find_first_child(
		tree_src_begin, tree_src_end, -1);

	assert(root_it != tree_src_end); // panic: no root

	add_node(tree_src_begin, tree_dst, root_it - tree_src_begin, -1, trans);

	// while there are new nodes added do
	for (Translator::const_iterator new_parent_it = trans.begin();
		new_parent_it != trans.end(); ++new_parent_it)
	{
		const NodeIdx parent_old_idx = *new_parent_it;
		const NodeIdx parent_new_idx = new_parent_it - trans.begin();

		// find all children of this node in the old tree
		for (TREE_CONST_ITER_T child_it = tree_src_begin;
			(child_it = find_first_child(child_it, tree_src_end, parent_old_idx)) != tree_src_end;
			++child_it)
		{
			// add the child to the new tree
			const NodeIdx child_idx = child_it - tree_src_begin;
			add_node(tree_src_begin, tree_dst, child_idx, parent_new_idx, trans);
		}
	}

	assert(trans.size() == size_t(tree_src_end - tree_src_begin));
	// panic: loops and/or multiple trees encountered
}


struct compute_arg
{
	pthread_t thread;
	size_t id;
	size_t offset;

	compute_arg() :
		thread(0)
	{}

	compute_arg(
		const size_t arg_id,
		const size_t arg_offs) :
		thread(0),
		id(arg_id),
		offset(arg_offs)
	{}
};

static void*
compute(
	void* arg)
{
	const compute_arg* const carg = reinterpret_cast< compute_arg* >(arg);
	const size_t id = carg->id;
	const size_t offset = carg->offset;

	pthread_barrier_wait(barrier + BARRIER_START);

	for (size_t i = 0; i < reps; ++i)
	{
		const size_t offs = i * offset;

		const Node* const tree_src_begin = tree + offs;
		const Node* const tree_src_end = tree + tree_size + offs;

		Tree tree_dst(pool_alloc< Node >(
			na + tree_size * id, tree_size));
		tree_dst.reserve(tree_size);

		Translator trans(pool_alloc< NodeIdx >(
			ta + tree_size * id, tree_size));
		trans.reserve(tree_size);

		// produce parent-list tree of breadth-first order
		bfs_tree(tree_src_begin, tree_src_end, tree_dst, trans);
	}

	pthread_barrier_wait(barrier + BARRIER_FINISH);

	return 0;
}


class workforce_t
{
	compute_arg record[one_less];
	bool successfully_init;

public:
	workforce_t(const size_t offset);
	~workforce_t();

	bool is_successfully_init() const
	{
		return successfully_init;
	}
};


static void
report_err(
	const char* const func,
	const int line,
	const size_t counter,
	const int err)
{
	std::cerr << func << ':' << line << ", i: "
		<< counter << ", err: " << err << std::endl;
}


workforce_t::workforce_t(
	const size_t offset) :
	successfully_init(false)
{
	for (size_t i = 0; i < sizeof(barrier) / sizeof(barrier[0]); ++i)
	{
		const int r = pthread_barrier_init(barrier + i, NULL, nthreads);

		if (0 != r)
		{
			report_err(__FUNCTION__, __LINE__, i, r);
			return;
		}
	}

	for (size_t i = 0; i < one_less; ++i)
	{
		record[i] = compute_arg(i + 1, offset);

		const int r = pthread_create(&record[i].thread, NULL, compute, record + i);

		if (0 != r)
		{
			report_err(__FUNCTION__, __LINE__, i, r);
			return;
		}
	}

	successfully_init = true;
}


workforce_t::~workforce_t()
{
	for (size_t i = 0; i < sizeof(barrier) / sizeof(barrier[0]); ++i)
	{
		const int r = pthread_barrier_destroy(barrier + i);

		if (0 != r)
			report_err(__FUNCTION__, __LINE__, i, r);
	}

	for (size_t i = 0; i < one_less && 0 != record[i].thread; ++i)
	{
		const int r = pthread_join(record[i].thread, NULL);

		if (0 != r)
			report_err(__FUNCTION__, __LINE__, i, r);
	}
}


static void
print_trees()
{
	for (size_t i = 0; i < nthreads; ++i)
	{
		const Node* it = na + i * tree_size;
		std::cout << it->second;

		for (; ++it != na + (i + 1) * tree_size;)
			std::cout << ", " << it->second;

		std::cout << std::endl;
	}
}


int
main(
	int argc,
	char** argv)
{
	size_t obfuscator; // invariance obfuscator
	std::ifstream in("tree.input");

	if (in.is_open())
	{
		in >> obfuscator;
		in.close();
	}
	else
	{
		std::cout << "enter 0: ";
		std::cin >> obfuscator;
	}

	const workforce_t workforce(obfuscator);

	if (!workforce.is_successfully_init())
	{
		std::cerr << "failed to raise workforce; bailing out" << std::endl;
		return -1;
	}

	// let the workforce start their engines
	const timespec ts = { 0, one_less * 100000000 };
	nanosleep(&ts, NULL);

	compute_arg carg(0, obfuscator);

	const uint64_t t0 = timer_nsec();

	compute(&carg);

	const uint64_t dt = timer_nsec() - t0;
	const double sec = double(dt) * 1e-9;

	std::cout << "elapsed time: " << sec << " s" << std::endl;

	print_trees();

	return 0;
}
