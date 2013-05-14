/*
 *	An Implementation of Place for Layout
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/place.cpp
 */
#include <sstream>
#include <cfloat>
#include <cmath>
#include <stdexcept>
#include <cstring>
#include <nana/gui/place.hpp>
#include <nana/gui/programming_interface.hpp>


namespace nana{	namespace gui
{
	//number_t is used to store a number type variable
	//such as integer, real and percent. Essentially, percent is a typo of real.
	class number_t
	{
	public:
		struct kind
		{
			enum t{integer, real, percent};
		};

		number_t()
			: kind_(kind::integer)
		{
			value_.integer = 0;
		}

		kind::t kind_of() const
		{
			return kind_;
		}

		int integer() const
		{
			if(kind::integer == kind_)
				return value_.integer;
			return static_cast<int>(value_.real);
		}

		double real() const
		{
			if(kind::integer == kind_)
				return value_.integer;
			return value_.real;
		}

		void assign(int i)
		{
			kind_ = kind::integer;
			value_.integer = i;
		}

		void assign(double d)
		{
			kind_ = kind::real;
			value_.real = d;
		}

		void assign_percent(double d)
		{
			kind_ = kind::percent;
			value_.real = d / 100;
		}
	private:
		kind::t kind_;
		union valueset
		{
			int integer;
			double real;
		}value_;
	};//end class number_t

	class tokenizer
	{
	public:
		struct token
		{
			enum t{
			div_start, div_end,
			identifier, vertical, grid, number, array,
			weight, gap,
			equal,
			eof, error
			};
		};

		tokenizer(const char* p)
			: divstr_(p), sp_(p)
		{}

		const std::string& idstr() const
		{
			return idstr_;
		}

		number_t number() const
		{
			return number_;
		}

		std::vector<number_t>& array()
		{
			return array_;
		}

		token::t read()
		{
			sp_ = _m_eat_whitespace(sp_);

			std::size_t readbytes = 0;
			switch(*sp_)
			{
			case '\0':
				return token::eof;
			case '=':
				++sp_;
				return token::equal;
			case '<':
				++sp_;
				return token::div_start;
			case '>':
				++sp_;
				return token::div_end;
			case '[':
				array_.clear();
				sp_ = _m_eat_whitespace(sp_ + 1);
				if(*sp_ == ']')
				{
					++sp_;
					return token::array;
				}

				while(true)
				{
					sp_ = _m_eat_whitespace(sp_);
					if(token::number != read())
						_m_throw_error("invalid array element");

					array_.push_back(number_);

					sp_ = _m_eat_whitespace(sp_);
					char ch = *sp_++;
					if(ch == ']')
						break;

					if(ch != ',')
						_m_throw_error("invalid array");
				}
				return token::array;
			case '.': case '-':
				if(*sp_ == '-')
				{
					readbytes = _m_number(sp_ + 1, true);
					if(readbytes)
						++ readbytes;
				}
				else
					readbytes = _m_number(sp_, false);

				if(readbytes)
				{
					sp_ += readbytes;
					return token::number;
				}
				else
					_m_throw_error(*sp_);
				break;
			default:
				if('0' <= *sp_ && *sp_ <= '9')
				{
					readbytes = _m_number(sp_, false);
					if(readbytes)
					{
						sp_ += readbytes;
						return token::number;
					}
				}
				break;
			}

			if('_' == *sp_ || isalpha(*sp_))
			{
				const char * idstart = sp_++;
				while('_' == *sp_ || isalpha(*sp_) || isalnum(*sp_))
					++sp_;

				idstr_.assign(idstart, sp_);

				if(idstr_ == "weight")
				{
					_m_attr_number_value();
					return token::weight;
				}
				else if(idstr_ == "gap")
				{
					_m_attr_number_value();
					return token::gap;
				}
				else if(idstr_ == "vertical")
					return token::vertical;
				else if(idstr_ == "grid")
					return token::grid;

				return token::identifier;
			}

			return token::error;
		}
	private:
		void _m_throw_error(char err_char)
		{
			std::stringstream ss;
			ss<<"place: invalid character '"<<err_char<<"' at "<<static_cast<unsigned>(sp_ - divstr_);
			throw std::runtime_error(ss.str());
		}

		void _m_attr_number_value()
		{
			if(token::equal != read())
				_m_throw_error("an equal sign is required after \'"+ idstr_ +"\'");

			const char* p = sp_;
			for(; *p == ' '; ++p);

			std::size_t len = 0;
			if(*p == '-')
			{
				len = _m_number(p + 1, true);
				if(len)	++len;
			}
			else
				len = _m_number(p, false);

			if(0 == len)
				_m_throw_error("the \'" + idstr_ + "\' requires a number(integer or real or percent)");

			sp_ += len + (p - sp_);
		}

		void _m_throw_error(const std::string& err)
		{
			std::stringstream ss;
			ss<<"place: "<<err<<" at "<<static_cast<unsigned>(sp_ - divstr_);
			throw std::runtime_error(ss.str());
		}

		const char* _m_eat_whitespace(const char* sp)
		{
			while(*sp && !isgraph(*sp))
				++sp;
			return sp;
		}

		std::size_t _m_number(const char* sp, bool negative)
		{
			const char* allstart = sp;
			sp = _m_eat_whitespace(sp);

			number_.assign(0);

			bool gotcha = false;
			int integer = 0;
			double real = 0;
			//read the integral part.
			const char* istart = sp;
			while('0' <= *sp && *sp <= '9')
			{
				integer = integer * 10 + (*sp - '0');
				++sp;
			}
			const char* iend = sp;

			if('.' == *sp)
			{
				double div = 1;
				const char* rstart = ++sp;
				while('0' <= *sp && *sp <= '9')
				{
					real += (*sp - '0') / (div *= 10);
					++sp;
				}

				if(rstart != sp)
				{
					real += integer;
					number_.assign(negative ? -real : real);
					gotcha = true;
				}
			}
			else if(istart != iend)
			{
				number_.assign(negative ? - integer : integer);
				gotcha = true;
			}

			if(gotcha)
			{
				for(;*sp == ' '; ++sp);
				if('%' == *sp)
				{
					if(number_t::kind::integer == number_.kind_of())
						number_.assign_percent(number_.integer());
					return sp - allstart + 1;
				}
				return sp - allstart;
			}
			return 0;
		}
	private:
		const char* divstr_;
		const char* sp_;
		std::string idstr_;
		number_t number_;
		std::vector<number_t> array_;
	};	//end class tokenizer


	//struct implement
	struct place::implement
	{
		class field_impl;
		class division;
		class div_arrange;
		class div_vertical_arrange;
		class div_grid;

		//this is used to answer the place resize event.
		struct place_resize_answer;

		window window_handle;
		event_handle event_size_handle;
		division * root_division;
		std::map<std::string, field_impl*> fields;

		implement()
			: window_handle(0), event_size_handle(0), root_division(0)
		{}

		//The following functions are defined behind the definition of class division.
		//because the class division here is an incomplete type.
		~implement();
		static division * search_div_name(division* start, const std::string&);
		division * scan_div(tokenizer&);
	};	//end struct implement


	place::field_t::~field_t(){}

	class place::implement::field_impl
		:	public place::field_t
	{
	public:
		struct element_t
		{
			struct kind
			{
				enum t{
				window, gap, fixed, percent, room
				};
			};

			kind::t kind_of_element;
			union
			{
				window handle;
				unsigned gap_value;
				fixed_t	*	fixed_ptr;
				percent_t *	percent_ptr;
				room_t	*	room_ptr;
			}u;

			element_t(window wd)
				: kind_of_element(kind::window)
			{
				u.handle = wd;
			}

			element_t(unsigned gap)
				: kind_of_element(kind::gap)
			{
				u.gap_value = gap;
			}

			element_t(const fixed_t& fixed)
				: kind_of_element(kind::fixed)
			{
				u.fixed_ptr = new fixed_t(fixed);
			}

			element_t(const percent_t& per)
				: kind_of_element(kind::percent)
			{
				u.percent_ptr = new percent_t(per);
			}

			element_t(const room_t& rm)
				: kind_of_element(kind::room)
			{
				u.room_ptr = new room_t(rm);
			}

			element_t(const element_t& rhs)
				: kind_of_element(rhs.kind_of_element)
			{
				switch(kind_of_element)
				{
				case kind::fixed:
					u.fixed_ptr = new fixed_t(*rhs.u.fixed_ptr);
					break;
				case kind::percent:
					u.percent_ptr = new percent_t(*rhs.u.percent_ptr);
					break;
				case kind::room:
					u.room_ptr = new room_t(*rhs.u.room_ptr);
					break;
				default:
					u = rhs.u;
					break;
				}
			}

			~element_t()
			{
				switch(kind_of_element)
				{
				case kind::fixed:
					delete u.fixed_ptr;
					break;
				case kind::percent:
					delete u.percent_ptr;
					break;
				case kind::room:
					delete u.room_ptr;
					break;
				default:	break;
				}
			}

			window window_handle() const
			{
				switch(kind_of_element)
				{
				case kind::window:
					return u.handle;
				case kind::fixed:
					return u.fixed_ptr->first;
				case kind::percent:
					return u.percent_ptr->first;
				case kind::room:
					return u.room_ptr->first;
				default:	break;
				}
				return 0;
			}
		};
	public:
		typedef std::vector<element_t>::iterator iterator;
		typedef std::vector<element_t>::const_iterator const_iterator;
		typedef std::vector<window>::iterator fastened_iterator;
		typedef std::vector<window>::const_iterator fastened_const_iterator;

		field_impl(place * p)
			: place_ptr_(p), attached(false)
		{}
	private:
		//A destroy handler for element
		struct element_destroy_handler
		{
			field_impl * field;

			element_destroy_handler(field_impl * p)
				: field(p)
			{}

			void operator()(const eventinfo& ei)
			{
				for(std::vector<element_t>::iterator i = field->elements.begin(), end = field->elements.end(); i != end; ++i)
				{
					if(ei.window != i->window_handle())
						continue;
					field->elements.erase(i);
					break;
				}
				field->place_ptr_->collocate();
			}
		};

		//A destroy handler for fastened window.
		//The deleting a fastened window does not change the layout.
		struct fasten_destroy_handler
		{
			field_impl * field;

			fasten_destroy_handler(field_impl * p)
				: field(p)
			{}

			void operator()(const eventinfo& ei)
			{
				for(std::vector<window>::iterator i = field->fastened.begin(), end = field->fastened.end(); i != end; ++i)
				{
					if(ei.window != *i)
						continue;
					field->fastened.erase(i);
					break;
				}
			}
		};

		//Listen to destroy of a window
		//It will delete the element and recollocate when the window destroyed.
		void _m_make_destroy(window wd)
		{
			API::make_event<events::destroy>(wd, element_destroy_handler(this));
		}

		field_t& operator<<(window wd)
		{
			elements.push_back(wd);
			_m_make_destroy(wd);
			return *this;
		}

		field_t& operator<<(unsigned gap)
		{
			elements.push_back(gap);
			return *this;
		}

		field_t& operator<<(const fixed_t& fx)
		{
			elements.push_back(fx);
			_m_make_destroy(fx.first);
			return *this;
		}

		field_t& operator<<(const percent_t& pcnt)
		{
			elements.push_back(pcnt);
			_m_make_destroy(pcnt.first);
			return *this;
		}

		field_t& operator<<(const room_t& r)
		{
			room_t x = r;
			if(x.second.first == 0)
				x.second.first = 1;
			if(x.second.second == 0)
				x.second.second = 1;
			elements.push_back(x);
			_m_make_destroy(r.first);
			return *this;
		}

		field_t& fasten(window wd)
		{
			fastened.push_back(wd);
			API::make_event<events::destroy>(wd, fasten_destroy_handler(this));
			return *this;
		}
	public:
		//returns the number of fixed pixels and the number of adjustable items
		std::pair<unsigned, std::size_t> fixed_and_adjustable() const
		{
			std::pair<unsigned, std::size_t> vpair;
			for(const_iterator i = elements.begin(), end = elements.end(); i != end; ++i)
			{
				switch(i->kind_of_element)
				{
				case element_t::kind::fixed:
					vpair.first += i->u.fixed_ptr->second;
					break;
				case element_t::kind::gap:
					vpair.first += i->u.gap_value;
					break;
				case element_t::kind::percent:	//the percent is not fixed and not adjustable.
					break;
				default:
					++vpair.second;
				}
			}
			return vpair;
		}

		unsigned percent_pixels(unsigned pixels) const
		{
			double perpx = 0;
			for(const_iterator i = elements.begin(), end = elements.end(); i != end; ++i)
			{
				if(element_t::kind::percent == i->kind_of_element)
					perpx += pixels * (i->u.percent_ptr->second / 100.0);
			}
			return static_cast<unsigned>(perpx);
		}
	public:
		bool attached;
		std::vector<element_t> elements;
		std::vector<window>	fastened;
	private:
		place * place_ptr_;
	};//end class field_impl

	class place::implement::division
	{
	public:
		struct kind
		{
			enum t{
			arrange, vertical_arrange, grid
			};
		};

		typedef std::vector<division*>::iterator iterator;
		typedef std::vector<division*>::const_iterator const_iterator;

		division(kind::t k, std::string& n)
			: kind_of_division(k), field(0)
		{
			name.swap(n);
		}

		virtual ~division()
		{
			//detach the field
			if(field)
				field->attached = false;

			for(std::vector<division*>::iterator i = children.begin(), end = children.end(); i != end; ++i)
			{
				delete (*i);
			}
		}

		bool is_fixed() const
		{
			return ((weight.kind_of() == number_t::kind::integer) && (weight.integer() != 0));
		}

		bool is_percent() const
		{
			return ((weight.kind_of() == number_t::kind::percent) && (weight.real() != 0));
		}

		//return the fixed pixels and adjustable items.
		std::pair<unsigned, std::size_t> fixed_pixels(kind::t match_kind) const
		{
			std::pair<unsigned, std::size_t> pair;
			if(field && (kind_of_division == match_kind))
				pair = field->fixed_and_adjustable();

			for(std::vector<division*>::const_iterator i = children.begin(), end = children.end(); i != end; ++i)
			{
				division * child = *i;
				if(false == child->is_fixed()) //it is adjustable
				{
					if(false == child->is_percent())
						++pair.second;
				}
				else
					pair.first += static_cast<unsigned>(child->weight.integer());
			}
			return pair;
		}

		virtual void collocate() = 0;

	public:
		kind::t kind_of_division;
		std::string name;
		std::vector<division*> children;
		nana::rectangle area;
		number_t weight;
		number_t gap;
		field_impl * field;
	};

	class place::implement::div_arrange
		: public division
	{
	public:
		div_arrange(std::string& name)
			: division(kind::arrange, name)
		{}

		virtual void collocate()
		{
			std::pair<unsigned, std::size_t> pair = fixed_pixels(kind::arrange);
			if(field)
				pair.first += field->percent_pixels(area.width);

			unsigned gap_size = static_cast<unsigned>(gap.kind_of() == number_t::kind::integer ? gap.integer() : area.width * gap.real());

			double percent_pixels = 0;

			for(iterator i = children.begin(), end = children.end(); i != end; ++i)
			{
				if((*i)->is_percent())
					percent_pixels += area.width * (*i)->weight.real();
			}

			pair.first += static_cast<unsigned>(percent_pixels);
			double adjustable_pixels = (pair.second && pair.first < area.width ? (double(area.width - pair.first) / pair.second) : 0.0);

			double left = area.x;
			for(iterator i = children.begin(), end = children.end(); i != end; ++i)
			{
				division * child = *i;

				child->area.x = static_cast<int>(left);
				child->area.y = area.y;
				child->area.height = area.height;

				double adj_px;
				if(false == child->is_fixed()) //the child is adjustable
				{
					if(false == child->is_percent())
					{
						adj_px = child->fixed_pixels(kind::arrange).first;
						if(adj_px <= adjustable_pixels)
							adj_px = adjustable_pixels;
					}
					else
						adj_px = static_cast<unsigned>(area.width * child->weight.real());
				}
				else
					adj_px = child->weight.integer();
				left += adj_px;
				child->area.width = static_cast<unsigned>(adj_px) - (static_cast<unsigned>(adj_px) > gap_size ? gap_size : 0);
				child->collocate();
			}

			if(field)
			{
				unsigned adj_px = static_cast<unsigned>(adjustable_pixels) - (static_cast<unsigned>(adjustable_pixels) > gap_size ? gap_size : 0);
				nana::rectangle r = area;

				for(field_impl::iterator i = field->elements.begin(), end = field->elements.end(); i != end; ++i)
				{
					field_impl::element_t & el = *i;

					r.x = static_cast<int>(left);
					typedef field_impl::element_t::kind ekind;
					switch(el.kind_of_element)
					{
					case ekind::fixed:
						r.width = el.u.fixed_ptr->second;
						API::move_window(el.u.fixed_ptr->first, r.x, r.y, r.width, r.height);
						left += r.width;
						break;
					case ekind::gap:
						left += el.u.gap_value;
						break;
					case ekind::percent:
						r.width = area.width * el.u.percent_ptr->second / 100;
						API::move_window(el.u.percent_ptr->first, r.x, r.y, r.width, r.height);
						left += r.width;
						break;
					case ekind::window:
						API::move_window(el.u.handle, r.x, r.y, adj_px, r.height);
						left += adjustable_pixels;
						break;
					case ekind::room:
						API::move_window(el.u.room_ptr->first, r.x, r.y, adj_px, r.height);
						left += adjustable_pixels;
						break;
					}
				}

				for(field_impl::fastened_iterator i = field->fastened.begin(), end = field->fastened.end(); i != end; ++i)
				{
					API::move_window(*i, area.x, area.y, area.width, area.height);
				}
			}
		}
	};//end class div_arrange

	class place::implement::div_vertical_arrange
		: public division
	{
	public:
		div_vertical_arrange(std::string& name)
			: division(kind::vertical_arrange, name)
		{}

		virtual void collocate()
		{
			std::pair<unsigned, std::size_t> pair = fixed_pixels(kind::vertical_arrange);
			if(field)
				pair.first += field->percent_pixels(area.height);

			unsigned gap_size = static_cast<unsigned>(gap.kind_of() == number_t::kind::integer ? gap.integer() : area.width * gap.real());

			double percent_pixels = 0;
			for(iterator i = children.begin(), end = children.end(); i != end; ++i)
			{
				if((*i)->is_percent())
					percent_pixels += area.height * (*i)->weight.real();
			}

			pair.first += static_cast<unsigned>(percent_pixels);
			double adjustable_pixels = (pair.second && pair.first < area.height ? (double(area.height - pair.first) / pair.second) : 0.0);


			double top = area.y;
			for(iterator i = children.begin(), end = children.end(); i != end; ++i)
			{
				division * child = *i;
				child->area.x = area.x;
				child->area.y = static_cast<int>(top);
				child->area.width = area.width;

				double adj_px;
				if(false == child->is_fixed()) //the child is adjustable
				{
					if(false == child->is_percent())
					{
						adj_px = child->fixed_pixels(kind::vertical_arrange).first;
						if(adj_px <= adjustable_pixels)
							adj_px = adjustable_pixels;
					}
					else
						adj_px = area.height * child->weight.real();
				}
				else
					adj_px = child->weight.integer();

				top += adj_px;
				child->area.height = static_cast<unsigned>(adj_px) - (static_cast<unsigned>(adj_px) > gap_size ? gap_size : 0);
				child->collocate();
			}

			if(field)
			{
				unsigned adj_px = static_cast<unsigned>(adjustable_pixels) - (static_cast<unsigned>(adjustable_pixels) > gap_size ? gap_size : 0);
				nana::rectangle r = area;
				for(field_impl::iterator i = field->elements.begin(), end = field->elements.end(); i != end; ++i)
				{
					field_impl::element_t & el = *i;
					r.y = static_cast<int>(top);
					typedef field_impl::element_t::kind ekind;
					switch(el.kind_of_element)
					{
					case ekind::fixed:
						r.height = el.u.fixed_ptr->second;
						API::move_window(el.u.fixed_ptr->first, r.x, r.y, r.width, r.height);
						top += r.height;
						break;
					case ekind::gap:
						top += el.u.gap_value;
						break;
					case ekind::percent:
						r.height = area.height * el.u.percent_ptr->second / 100;
						API::move_window(el.u.percent_ptr->first, r.x, r.y, r.width, r.height);
						top += r.height;
						break;
					case ekind::window:
						API::move_window(el.u.handle, r.x, r.y, r.width, adj_px);
						top += adjustable_pixels;
						break;
					case ekind::room:
						API::move_window(el.u.room_ptr->first, r.x, r.y, r.width, adj_px);
						top += adjustable_pixels;
						break;
					}
				}

				for(field_impl::fastened_iterator i = field->fastened.begin(), end = field->fastened.end(); i != end; ++i)
				{
					API::move_window(*i, area.x, area.y, area.width, area.height);
				}
			}
		}
	};//end class div_vertical_arrange

	class place::implement::div_grid
		: public division
	{
	public:
		div_grid(std::string& name)
			: division(kind::grid, name)
		{
			dimension.first = dimension.second = 0;
		}

		virtual void collocate()
		{
			if(0 == field)
				return;

			unsigned gap_size = (gap.kind_of() == number_t::kind::percent ?
				static_cast<unsigned>(area.width * gap.real()) : static_cast<unsigned>(gap.integer()));

			if(dimension.first <= 1 && dimension.second <= 1)
			{
				std::size_t n_of_wd = _m_number_of_window();

				std::size_t edge;
				switch(n_of_wd)
				{
				case 0:
				case 1:
					edge = 1;	break;
				case 2: case 3: case 4:
					edge = 2;	break;
				default:
					edge = static_cast<std::size_t>(std::sqrt(double(n_of_wd)));
					if((edge * edge) < n_of_wd) ++edge;
				}

				bool exit_for = false;
				double y = area.y;
				double block_w = area.width / double(edge);
				double block_h = area.height / double((n_of_wd / edge) + (n_of_wd % edge ? 1 : 0));
				unsigned uns_block_w = static_cast<unsigned>(block_w);
				unsigned uns_block_h = static_cast<unsigned>(block_h);
				unsigned height = (uns_block_h > gap_size ? uns_block_h - gap_size : uns_block_h);

				field_impl::const_iterator i = field->elements.begin(), end = field->elements.end();

				for(std::size_t u = 0; u < edge; ++u)
				{
					double x = area.x;
					for(std::size_t v = 0; v < edge; ++v)
					{
						i = _m_search(i, end);
						if(i == end)
						{
							exit_for = true;
							break;
						}

						window wd = 0;
						unsigned value = 0;
						typedef field_impl::element_t::kind ekind;
						switch(i->kind_of_element)
						{
						case ekind::fixed:
							wd = i->u.fixed_ptr->first;
							value = i->u.fixed_ptr->second;
							break;
						case ekind::percent:
							wd = i->u.percent_ptr->first;
							value = i->u.percent_ptr->second * area.width / 100;
							break;
						case ekind::window:
							wd = i->u.handle;
							value = static_cast<unsigned>(block_w);
							break;
						default:	break;
						}
						++i;

						unsigned width = (value > uns_block_w ? uns_block_w : value);
						if(width > gap_size)	width -= gap_size;
						API::move_window(wd, static_cast<int>(x), static_cast<int>(y), width, height);
						x += block_w;
					}
					if(exit_for) break;
					y += block_h;
				}
			}
			else
			{
				double block_w = area.width / double(dimension.first);
				double block_h = area.height / double(dimension.second);

				char *table = new char[dimension.first * dimension.second];

				std::memset(table, 0, dimension.first * dimension.second);

				std::size_t lbp = 0;
				bool exit_for = false;

				field_impl::const_iterator i = field->elements.begin(), end = field->elements.end();

				for(std::size_t c = 0; c < dimension.second; ++c)
				{
					for(std::size_t l = 0; l < dimension.first; ++l)
					{
						if(table[l + lbp])
							continue;

						i = _m_search(i, end);
						if(i == end)
						{
							exit_for = true;
							break;
						}

						typedef field_impl::element_t::kind ekind;
						std::pair<unsigned, unsigned> room(1, 1);

						if(i->kind_of_element == ekind::room)
						{
							room = i->u.room_ptr->second;
							if(room.first > dimension.first - l)
								room.first = dimension.first - l;
							if(room.second > dimension.second - c)
								room.second = dimension.second - c;
						}

						window wd = 0;
						switch(i->kind_of_element)
						{
						case ekind::fixed:
							wd = i->u.fixed_ptr->first;
							break;
						case ekind::percent:
							wd = i->u.percent_ptr->first;
							break;
						case ekind::window:
							wd = i->u.handle;
							break;
						default:	break;
						}

						int pos_x = area.x + static_cast<int>(l * block_w);
						int pos_y = area.y + static_cast<int>(c * block_h);
						if(room.first <= 1 && room.second <= 1)
						{
							unsigned width = static_cast<unsigned>(block_w), height = static_cast<unsigned>(block_h);
							if(width > gap_size)	width -= gap_size;
							if(height > gap_size)	height -= gap_size;

							API::move_window(wd, pos_x, pos_y, width, height);
							table[l + lbp] = 1;
						}
						else
						{
							unsigned width = static_cast<unsigned>(block_w * room.first), height = static_cast<unsigned>(block_h * room.second);
							if(width > gap_size)	width -= gap_size;
							if(height > gap_size)	height -= gap_size;

							API::move_window(i->u.room_ptr->first, pos_x, pos_y, width, height);

							for(std::size_t y = 0; y < room.second; ++y)
								for(std::size_t x = 0; x < room.first; ++x)
								{
									table[l + x + lbp + y * dimension.first] = 1;
								}
						}
						++i;
					}

					if(exit_for)
						break;

					lbp += dimension.first;
				}
				delete [] table;
			}

			for(field_impl::fastened_iterator i = field->fastened.begin(), end = field->fastened.end(); i != end; ++i)
			{
				API::move_window(*i, area.x, area.y, area.width, area.height);
			}
		}
	private:
		static field_impl::const_iterator _m_search(field_impl::const_iterator i, field_impl::const_iterator end)
		{
			if(i == end) return end;

			while(i->kind_of_element == field_impl::element_t::kind::gap)
			{
				if(++i == end) return end;
			}
			return i;
		}

		std::size_t _m_number_of_window() const
		{
			if(0 == field) return 0;

			std::size_t n = 0;
			for(field_impl::iterator i = field->elements.begin(), end = field->elements.end(); i != end; ++i)
			{
				if(field_impl::element_t::kind::gap != i->kind_of_element)
					++n;
			}
			return n;
		}
	public:
		std::pair<unsigned, unsigned> dimension;
	};//end class div_grid

	place::implement::~implement()
	{
		API::umake_event(event_size_handle);
		delete root_division;
		for(std::map<std::string, field_impl*>::iterator i = fields.begin(), end = fields.end(); i != end; ++i)
		{
			delete i->second;
		}
	}

	//search_div_name
	//search a division with the specified name.
	place::implement::division * place::implement::search_div_name(division* start, const std::string& name)
	{
		if(0 == start) return 0;

		if(start->name == name) return start;

		for(division::iterator i = start->children.begin(), end = start->children.end(); i != end; ++i)
		{
			division * div = search_div_name(*i, name);
			if(div)
				return div;
		}
		return 0;
	}

	place::implement::division* place::implement::scan_div(tokenizer& tknizer)
	{
		typedef tokenizer::token token;

		division * div = 0;
		token::t div_type = token::eof;
		std::string name;

		number_t weight;
		number_t gap;
		std::vector<number_t> array;

		std::vector<division*> children;
		for(token::t tk = tknizer.read(); tk != token::eof; tk = tknizer.read())
		{
			bool exit_for = false;
			switch(tk)
			{
			case token::div_start:
				children.push_back(scan_div(tknizer));
				break;
			case token::vertical:
			case token::grid:
				div_type = tk;
				break;
			case token::array:
				tknizer.array().swap(array);
				break;
			case token::weight:
				weight = tknizer.number();
				//If the weight is type of real, convert it to integer.
				//the integer and percent are allowed for weight.
				if(weight.kind_of() == number_t::kind::real)
					weight.assign(static_cast<int>(weight.real()));
				break;
			case token::gap:
				gap = tknizer.number();
				//If the gap is type of real, convert it to integer.
				//the integer and percent are allowed for gap.
				if(gap.kind_of() == number_t::kind::real)
					gap.assign(static_cast<int>(gap.real()));
				break;
			case token::div_end:
				exit_for = true;
				break;
			case token::identifier:
				name = tknizer.idstr();
				break;
            default:    break;
			}
			if(exit_for)
				break;
		}

		field_impl * field = 0;
		if(name.size())
		{
			//find the field with specified name.
			//the field may not be created.
			std::map<std::string, field_impl*>::iterator i = fields.find(name);
			if(fields.end() != i)
			{
				field = i->second;

				//the field is attached to a division, it means there is another division with same name.
				if(field->attached)
					throw std::runtime_error("place, the name \'"+ name +"\' is redefined.");

				//this field will be attached to the division that will be created later.
				field->attached = true;
			}
		}

		switch(div_type)
		{
		case token::eof:
			div = new div_arrange(name);
			break;
		case token::vertical:
			div = new div_vertical_arrange(name);
			break;
		case token::grid:
			{
				div_grid * p = new div_grid(name);

				if(array.size())
				{
					if(array[0].kind_of() != number_t::kind::percent)
						p->dimension.first = array[0].integer();
				}

				if(array.size() > 1)
				{
					if(array[1].kind_of() != number_t::kind::percent)
						p->dimension.second = array[1].integer();
				}

				if(0 == p->dimension.first)
					p->dimension.first = 1;

				if(0 == p->dimension.second)
					p->dimension.second = 1;

				div = p;
			}
			break;
        default:
            throw std::runtime_error("nana.place: invalid division type.");
		}

		div->weight = weight;
		div->gap = gap;
		div->field = field;		//attach the field to the division
		div->children.swap(children);
		return div;
	}

	//class place
		place::place()
			: impl_(new implement)
		{}

		struct place::implement::place_resize_answer
		{
			implement * impl;

			place_resize_answer(implement* arg)
				:impl(arg)
			{}

			void operator()(const eventinfo& ei)
			{
				if(impl->root_division)
				{
					impl->root_division->area = API::window_size(ei.window);
					impl->root_division->collocate();
				}
			}
		};

		place::place(window wd)
			: impl_(new implement)
		{
			bind(wd);
		}

		place::~place()
		{
			delete impl_;
		}

		void place::bind(window wd)
		{
			if(impl_->window_handle)
				throw std::runtime_error("place.bind: it has already bind to a window.");

			impl_->window_handle = wd;
			impl_->event_size_handle = API::make_event<events::size>(wd, implement::place_resize_answer(impl_));
		}

		void place::div(const char* s)
		{
			delete impl_->root_division;
			impl_->root_division = 0;

			tokenizer tknizer(s);
			impl_->root_division = impl_->scan_div(tknizer);
		}

		place::fixed_t place::fixed(window wd, unsigned size)
		{
			return fixed_t(wd, size);
		}

		place::percent_t place::percent(window wd, int per)
		{
			return percent_t(wd, per);
		}

		place::room_t place::room(window wd, unsigned w, unsigned h)
		{
			return room_t(wd, std::pair<unsigned, unsigned>(w, h));
		}

		place::field_reference place::field(const char* name)
		{
			name = name ? name : "";

			//get the field with specified name, if no such field with specified name
			//then create one.
			implement::field_impl * & p = impl_->fields[name];
			if(0 == p)
				p = new implement::field_impl(this);

			if((false == p->attached) && impl_->root_division)
			{
				//search the division with the specified name,
				//and attached the division to the field
				implement::division * div = implement::search_div_name(impl_->root_division, name);
				if(div)
				{
					if(div->field && (div->field != p))
						throw std::runtime_error("nana.place: unexpected error, the division attachs a unexpected field.");

					div->field = p;
					p->attached = true;
				}
			}
			return *p;
		}

		void place::collocate()
		{
			if(impl_->root_division && impl_->window_handle)
			{
				impl_->root_division->area = API::window_size(impl_->window_handle);
				impl_->root_division->collocate();

				for(std::map<std::string, implement::field_impl*>::iterator i = impl_->fields.begin(), end = impl_->fields.end(); i != end; ++i)
				{
					bool is_show = i->second->attached;
					if(is_show)
						is_show = (0 != implement::search_div_name(impl_->root_division, i->first));

					for(std::vector<implement::field_impl::element_t>::iterator u = i->second->elements.begin(), uend = i->second->elements.end(); u != uend; ++u)
						API::show_window(u->window_handle(), is_show);
				}
			}
		}
	//end class place

}//end namespace gui
}//end namespace nana
