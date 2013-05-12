/*
 *	A List Box Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/listbox.cpp
 *	@patchs:
 *		Jan 03 2012, unsigned to std::size_t conversion fail for x64, Hiroshi Seki
 */

#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/scroll.hpp>
#include <nana/paint/gadget.hpp>
#include <list>
#include <deque>
#include <stdexcept>
#include <sstream>

namespace nana{ namespace gui{
	namespace drawerbase
	{
		namespace listbox
		{
			class es_header
			{
			public:
				typedef std::size_t size_type;
				struct item_t
				{
					nana::string text;
					unsigned pixels;
					bool visible;
					size_type index;
					nana::functor<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool reverse)> weak_ordering;
				};

				typedef std::vector<item_t> container;

				es_header()
					:visible_(true)
				{}

				bool visible() const
				{
					return visible_;
				}

				bool visible(bool v)
				{
					if(visible_ != v)
					{
						visible_ = v;
						return true;
					}
					return false;
				}

				nana::functor<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool reverse)> fetch_comp(std::size_t index) const
				{
					for(container::const_iterator i = cont_.begin(), end = cont_.end(); i != end; ++i)
					{
						if(i->index == index)
							return i->weak_ordering;
					}
					return nana::functor<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool)>();
				}

				void create(const nana::string& text, unsigned pixels)
				{
					item_t m;
					m.text = text;
					m.pixels = pixels;
					m.visible = true;
					m.index = static_cast<size_type>(cont_.size());
					cont_.push_back(m);
				}

				void item_width(size_type index, unsigned width)
				{
					if(index < cont_.size())
					{
						for(container::iterator i = cont_.begin(); i != cont_.end(); ++i)
						{
							if(i->index == index)
								i->pixels = width;
						}
					}
				}

				unsigned pixels() const
				{
					unsigned pixels = 0;
					for(container::const_iterator i = cont_.begin(), end = cont_.end(); i != end; ++i)
					{
						if(i->visible)
							pixels += i->pixels;
					}
					return pixels;
				}

				size_type index(size_type n) const
				{
					return (n < cont_.size() ? cont_[n].index : npos);
				}

				const container& cont() const
				{
					return cont_;
				}

				item_t& get_item(size_type index)
				{
					for(container::iterator i = cont_.begin(), end = cont_.end(); i != end; ++i)
					{
						if(i->index == index)
							return *i;
					}
					throw std::out_of_range("Nana.GUI.Listbox: invalid header index.");
				}

				const item_t& get_item(size_type index) const
				{
					for(container::const_iterator i = cont_.begin(); i != cont_.end(); ++i)
					{
						if(i->index == index)
							return *i;
					}
					throw std::out_of_range("Nana.GUI.Listbox: invalid header index.");
				}

				size_type item_by_x(int x) const
				{
					for(container::const_iterator i = cont_.begin(); i != cont_.end(); ++i)
					{
						if(x < static_cast<int>(i->pixels))
							return i->index;
						x -= i->pixels;
					}
					return npos;
				}

				bool item_pos(size_type index, int& xpos, unsigned& pixels) const
				{
					xpos = 0;
					for(container::const_iterator i = cont_.begin(); i != cont_.end(); ++i)
					{
						if(i->index == index)
						{
							pixels = i->pixels;
							return true;
						}
						else if(i->visible)
							xpos += i->pixels;
					}
					return true;
				}

				int xpos(size_type index) const
				{
					int x = 0;
					for(container::const_iterator i = cont_.begin(); i != cont_.end(); ++i)
					{
						if(i->index == index)
							break;
						else if(i->visible)
							x += i->pixels;
					}
					return x;
				}

				size_type neighbor(size_type index, bool front) const
				{
					size_type n = npos;
					for(container::const_iterator i = cont_.begin(); i != cont_.end(); ++i)
					{
						if(i->index == index)
						{
							if(front)	return n;
							for(++i; i != cont_.end(); ++i)
							{
								if(i->visible) return i->index;
							}
							break;
						}
						else if(i->visible)
							n = i->index;
					}
					return npos;
				}

				size_type begin() const
				{
					for(container::const_iterator i = cont_.begin(); i != cont_.end(); ++i)
					{
						if(i->visible) return i->index;
					}
					return npos;
				}

				size_type last() const
				{
					for(container::const_reverse_iterator i = cont_.rbegin(); i != cont_.rend(); ++i)
					{
						if(i->visible) return i->index;
					}
					return npos;
				}

				void move(size_type index, size_type to, bool front)
				{
					if((index != to) && (index < cont_.size()) && (to < cont_.size()))
					{
						item_t from;
						for(container::iterator i = cont_.begin(), end = cont_.end(); i != end;  ++i)
						{
							if(index == i->index)
							{
								from = *i;
								cont_.erase(i);
								break;
							}
						}

						for(container::iterator i = cont_.begin(), end = cont_.end(); i != end;  ++i)
						{
							if(to == i->index)
							{
								cont_.insert((front ? i : ++i), from);
								break;
							}
						}
					}
				}
			private:
				bool visible_;
				container cont_;
			};

			class es_lister
			{
			public:
				struct item_t
				{
					typedef std::vector<nana::string> container;

					container texts;
					color_t bkcolor;
					color_t fgcolor;
					nana::paint::image img;

					struct flags_tag
					{
						bool selected:1;
						bool checked:1;
					}flags;
					mutable nana::any * anyobj;

					item_t()
						:bkcolor(0xFF000000), fgcolor(0xFF000000), anyobj(0)
					{
						flags.selected = flags.checked = false;
					}

					item_t(const item_t& r)
						:	texts(r.texts), bkcolor(r.bkcolor), fgcolor(r.fgcolor), img(r.img),
							flags(r.flags), anyobj(r.anyobj ? new nana::any(*r.anyobj) : 0)
					{}

					~item_t()
					{
						delete anyobj;
					}

					item_t& operator=(const item_t& r)
					{
						if(this != &r)
						{
							texts = r.texts;
							flags = r.flags;
							anyobj = (r.anyobj? new nana::any(*r.anyobj) : 0);
							bkcolor = r.bkcolor;
							fgcolor = r.fgcolor;
							img = r.img;
						}
						return *this;
					}
				};

				struct category
				{
					typedef std::deque<item_t> container;

					nana::string text;
					std::vector<std::size_t> sorted;
					container items;
					bool expand;

					bool select() const
					{
						for(container::const_iterator i = items.begin(), end = items.end(); i != end; ++i)
						{
							if(i->flags.selected == false) return false;
						}
						return (items.size() != 0);
					}
				};

				//The default compare.
				struct weak_ordering
				{
					category& cat;
					bool neg;
					std::size_t si;

					weak_ordering(category& cat, bool neg, std::size_t si)
						: cat(cat), neg(neg), si(si)
					{}
					bool operator()(std::size_t x, std::size_t y)
					{
						return (neg ? cat.items[x].texts[si] > cat.items[y].texts[si] : cat.items[x].texts[si] < cat.items[y].texts[si]);
					}
				};

				//A proxy of user-defined compare
				struct weak_ordering_udcomp
				{
					category& cat;
					bool neg;
					std::size_t si;
					typedef nana::functor<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool reverse)> compare_t;
					compare_t comp;
	
					weak_ordering_udcomp(category& cat, bool neg, std::size_t si, const compare_t& comp)
						: cat(cat), neg(neg), si(si), comp(comp)
					{}

					bool operator()(std::size_t x, std::size_t y)
					{
						item_t & mx = cat.items[x];
						item_t & my = cat.items[y];
						const nana::string& a = mx.texts[si];
						const nana::string& b = my.texts[si];
						return comp(a, mx.anyobj, b, my.anyobj, neg);
					}
				};

			public:
				typedef std::size_t size_type;
				typedef std::list<category> container;
				mutable extra_events ext_event;

				nana::functor<nana::functor<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool reverse)>(std::size_t)> fetch_ordering_comparer;

				es_lister()
					: widget_(0), sorted_index_(npos), sorted_reverse_(false)
				{
					category cg;
					cg.expand = true;
					list_.push_back(cg);
				}

				void bind(widget& wd)
				{
					widget_ = dynamic_cast<gui::listbox*>(&wd);
					if(0 == widget_)
						throw std::bad_cast();
				}

				gui::listbox* wd_ptr() const
				{
					return widget_;
				}

				nana::any * anyobj(size_type cat, size_type index, bool allocate_if_empty) const
				{
					container::const_iterator i = _m_at(cat);
					if(index < i->items.size())
					{
						const item_t & item = i->items[index];
						if(item.anyobj)
							return item.anyobj;
						if(allocate_if_empty)
							return (item.anyobj = new nana::any);
					}
					return 0;
				}

				void sort()
				{
					if(sorted_index_ != npos)
					{
						nana::functor<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool reverse)> comp = fetch_ordering_comparer(sorted_index_);
						container::iterator i = list_.begin(), end = list_.end();
						if(comp)
						{
							for(; i != end; ++i)
							{
								container::value_type & cat = *i;
								std::sort(cat.sorted.begin(), cat.sorted.end(), weak_ordering_udcomp(cat, sorted_reverse_, sorted_index_, comp));
							}
						}
						else
						{
							for(; i != end; ++i)
							{
								container::value_type & cat = *i;
								std::sort(cat.sorted.begin(), cat.sorted.end(), weak_ordering(cat, sorted_reverse_, sorted_index_));
							}
						}
					}
				}

				bool sort_index(std::size_t index)
				{
					if(npos != index)
					{
						if(index != sorted_index_)
						{
							sorted_index_ = index;
							sorted_reverse_ = false;
						}
						else
							sorted_reverse_ = !sorted_reverse_;

						sort();
						return true;
					}
					sorted_index_ = npos;
					return false;
				}

				std::size_t sort_index() const
				{
					return sorted_index_;
				}

				bool sort_reverse() const
				{
					return sorted_reverse_;
				}

				void create(const nana::string& text)
				{
					category cg;
					cg.expand = true;
					cg.text = text;
					list_.push_back(cg);
				}

				void push_back(size_type cat, const nana::string& text)
				{
					item_t item;
					item.texts.push_back(text);
					container::iterator i = _m_at(cat);
					i->items.push_back(item);
					i->sorted.push_back(i->items.size() - 1);
				}

				bool insert(size_type cat, size_type index, const nana::string& text)
				{
					container::iterator i = _m_at(cat);
					item_t item;
					item.texts.push_back(text);
					if(index < i->items.size())
						i->items.insert(i->items.begin() + index, item);
					else if(index == i->items.size())
						i->items.push_back(item);
					else
						return false;
					i->sorted.push_back(i->items.size() - 1);
					return true;
				}

				category::container::value_type& at(size_type cat, size_type index)
				{
					if(sorted_index_ != npos)
						index = absolute(cat, index);
					return _m_at(cat)->items.at(index);
				}

				const category::container::value_type& at(size_type cat, size_type index) const
				{
					if(sorted_index_ != npos)
						index = absolute(cat, index);
					return _m_at(cat)->items.at(index);
				}

				category::container::value_type& at_abs(size_type cat, size_type index)
				{
					return _m_at(cat)->items.at(index);
				}

				void clear(size_type cat)
				{
					container::value_type & catobj = *_m_at(cat);
					catobj.items.clear();
					catobj.sorted.clear();
				}

				void clear()
				{
					for(container::iterator i = list_.begin(), end = list_.end(); i != end; ++i)
					{
						i->items.clear();
						i->sorted.clear();
					}
				}

				std::pair<size_type, size_type> advance(size_type cat, size_type index, size_type n)
				{
					std::pair<size_type, size_type> dpos(npos, npos);
					if(cat >= size_categ() || (index != npos && index >= size_item(cat))) return dpos;

					dpos.first = cat;
					dpos.second = index;

					while(n)
					{
						if(dpos.second == npos)
						{
							if(expand(dpos.first) == false)
							{
								if(dpos.first + 1 == size_categ())
									break;
								++dpos.first;
							}
							else
								dpos.second = 0;
							--n;
						}
						else
						{
							size_type rest = size_item(dpos.first) - dpos.second - 1;
							if(rest == 0)
							{
								if(dpos.first + 1 == size_categ())
									break;
								++dpos.first;
								dpos.second = npos;
								--n;
							}
							else if(rest < n)
							{
								n -= rest;
								if(dpos.first + 1 >= size_categ())
								{
									dpos.second += rest;
									break;
								}
								dpos.second = npos;
								++dpos.first;
							}
							else
							{
								dpos.second += n;
								break;
							}
						}
					}
					return dpos;
				}

				size_type distance(size_type cat, size_type index, size_type to_cat, size_type to_index) const
				{
					if(cat == to_cat && index == to_index) return 0;

					if(to_cat == cat)
					{
						if(index > to_index && index != npos)
							std::swap(index, to_index);

						return (index == npos ? to_index + 1 : to_index - index);
					}
					else if(to_cat < cat)
					{
						std::swap(cat, to_cat);
						std::swap(index, to_index);
					}

					size_type n = 0;
					container::const_iterator i = _m_at(cat);
					if(index == npos)
					{
						if(i->expand)
							n = i->items.size();
					}
					else
						n = i->items.size() - (index + 1);

					for(++i, ++cat; i != list_.end(); ++i, ++cat)
					{
						++n; //this is a category
						if(cat != to_cat)
						{
							if(i->expand)
								n += i->items.size();
						}
						else
						{
							if(to_index != npos)
								n += (to_index + 1);
							break;
						}
					}
					return n;
				}

				void text(size_type cat, size_type index, size_type subitem, const nana::string& str, size_type header_size)
				{
					if(subitem < header_size)
					{
						container::iterator i = _m_at(cat);
						if(index < i->items.size())
						{
							std::vector<nana::string> & cont = i->items[index].texts;
							if(subitem >= cont.size())
							{	//If the index of specified sub item is over the number of sub items that item contained,
								//it fills the non-exist items.
								cont.resize(subitem);
								cont.push_back(str);
							}
							else
							{
								cont[subitem] = str;
								if(sorted_index_ == subitem)
									sort();
							}
						}
					}
				}

				void erase(size_type cat, size_type index)
				{
					container::iterator i = _m_at(cat);
					if(index < i->items.size())
					{
						i->items.erase(i->items.begin() + index);
						i->sorted.erase(std::find(i->sorted.begin(), i->sorted.end(), i->items.size()));
						sort();
					}
				}

				void erase(size_type cat)
				{
					container::iterator i = _m_at(cat);
					//If the category is the first one, it just clears the items instead of removing whole category.
					if(cat)
					{
						list_.erase(i);
					}
					else
					{
						i->items.clear();
						i->sorted.clear();
					}
				}

				void erase()
				{
					container::iterator i = list_.begin();
					i->items.clear();
					i->sorted.clear();
					if(list_.size() > 1)
						list_.erase(++i, list_.end());
				}

				bool expand(size_type cat, bool exp)
				{
					if(cat)
					{
						bool & expanded = _m_at(cat)->expand;
						if(expanded != exp)
						{
							expanded = exp;
							return true;
						}
					}
					return false;
				}

				bool expand(size_type cat) const
				{
					if(cat)
						return _m_at(cat)->expand;
					return false;
				}

				const std::list<category>& cat_container() const
				{
					return list_;
				}

				size_type the_number_of_expanded() const
				{
					size_type n = list_.size() - 1;

					for(std::list<category>::const_iterator i = list_.begin(); i != list_.end(); ++i)
					{
						if(i->expand)
							n += i->items.size();
					}
					return n;
				}

				void check_for_all(bool chk)
				{
					size_type cat = 0;
					for(container::iterator i = list_.begin(); i != list_.end(); ++i, ++cat)
					{
						size_type index = 0;
						for(category::container::iterator u = i->items.begin(); u != i->items.end(); ++u, ++index)
						{
							if(u->flags.checked != chk)
							{
								u->flags.checked = chk;
								ext_event.checked(*widget_, cat, index, chk);
							}
						}
					}
				}

				void item_checked(std::vector<std::pair<size_type, size_type> >& vec) const
				{
					size_type categ_idx = 0;
					size_type index = 0;
					for(container::const_iterator i = list_.begin(); i != list_.end(); ++i, ++categ_idx)
					{
						index = 0;
						const category& categ = *i;
						for(category::container::const_iterator u = categ.items.begin(); u != categ.items.end(); ++u, ++index)
						{
							if(u->flags.checked)
								vec.push_back(std::pair<size_type, size_type>(categ_idx, index));
						}
					}
				}

				bool select_for_all(bool sel)
				{
					bool changed = false;
					std::size_t cat = 0;
					for(container::iterator i = list_.begin(); i != list_.end(); ++i)
					{
						std::size_t index = 0;
						for(category::container::iterator u = i->items.begin(); u != i->items.end(); ++u, ++index)
							if(u->flags.selected != sel)
							{
								changed = true;
								u->flags.selected = sel;
								ext_event.selected(*widget_, cat, index, sel);
							}
						++cat;
					}
					return changed;
				}

				void item_selected(std::vector<std::pair<size_type, size_type> >& vec) const
				{
					size_type categ_idx = 0;
					size_type index = 0;
					for(container::const_iterator i = list_.begin(); i != list_.end(); ++i, ++categ_idx)
					{
						index = 0;
						const category& categ = *i;
						for(category::container::const_iterator u = categ.items.begin(); u != categ.items.end(); ++u, ++index)
						{
							if(u->flags.selected)
								vec.push_back(std::pair<size_type, size_type>(categ_idx, index));
						}
					}
				}

				void move_select(bool upwards)
				{
					std::vector<std::pair<size_type, size_type> > svec;
					item_selected(svec);

					//get the start pos for moving.
					std::pair<size_type, size_type> spos;
					if(svec.size())
					{
						select_for_all(false);
						spos = svec[0];
					}
					else
					{
						bool good = false;
						for(std::size_t i = 0; i < list_.size(); ++i)
						{
							if(size_item(i))
							{
								spos.first = i;
								spos.second = 0;
								good = true;
								break;
							}
						}
						if(good == false) return;
					}

					//start moving
					while(true)
					{
						if(upwards == false)
						{
							if(good(spos.first))
							{
								if(size_item(spos.first) > spos.second)
								{
									++spos.second;
								}
								else if(size_categ() > spos.first)
								{
									++spos.first;
									spos.second = 0;
								}
							}
							else
							{
								spos.first = 0;
								spos.second = 0;
							}
						}
						else
						{
							if(spos.second == 0)
							{
								//there is an item at least definitely, because the start pos is an available item.
								do
								{
									if(spos.first == 0)
										spos.first = size_categ() - 1;
									else
										--spos.first;
								}
								while(0 == size_item(spos.first));

								spos.second = size_item(spos.first) - 1;
							}
							else
								--spos.second;
						}

						if(good(spos.first))
						{
							if(expand(spos.first) == false)
								expand(spos.first, true);

							if(good(spos.first, spos.second))
							{
								at(spos.first, spos.second).flags.selected = true;
								ext_event.selected(*widget_, spos.first, absolute(spos.first, spos.second), true);
							}
							else	break;
						}
						else	break;
					}
				}

				size_type size_categ() const
				{
					return list_.size();
				}

				size_type size_item(size_type cat) const
				{
					return _m_at(cat)->items.size();
				}

				nana::string item_text(size_type categ, size_type index, size_type sub) const
				{
					if(categ < list_.size())
					{
						container::const_iterator i = list_.begin();
						std::advance(i, categ);
						if(index < i->items.size() && (sub < i->items[index].texts.size()))
							return i->items[index].texts[sub];
					}
					return nana::string();
				}

				bool categ_checked(size_type cat) const
				{
					category::container const & items = _m_at(cat)->items;
					for(category::container::const_iterator i = items.begin(); i != items.end(); ++i)
					{
						if(i->flags.checked == false)
							return false;
					}
					return true;
				}

				bool categ_checked(size_type cat, bool chk)
				{
					bool changed = false;
					category::container & items = _m_at(cat)->items;
					size_type index = 0;
					for(category::container::iterator i = items.begin(); i != items.end(); ++i, ++index)
						if(i->flags.checked != chk)
						{
							i->flags.checked = chk;
							ext_event.checked(*widget_, cat, index, chk);
							changed = true;
						}
					return changed;
				}

				bool categ_checked_reverse(size_type cat)
				{
					if(list_.size() > cat)
						return categ_checked(cat, !categ_checked(cat));
					return false;
				}

				bool categ_selected(size_type cat) const
				{
					const category::container & items = _m_at(cat)->items;
					for(category::container::const_iterator i = items.begin(); i != items.end(); ++i)
					{
						if(i->flags.selected == false)
							return false;
					}
					return true;
				}

				bool categ_selected(size_type cat, bool sel)
				{
					bool changed = false;
					category::container & items = _m_at(cat)->items;
					size_type index = 0;
					for(category::container::iterator i = items.begin(); i != items.end(); ++i, ++index)
					{
						if(i->flags.selected != sel)
						{
							i->flags.selected = sel;
							ext_event.selected(*widget_, cat, index, sel);
							changed = true;
						}
					}
					return changed;
				}

				void reverse_categ_selected(size_type categ)
				{
					categ_selected(categ, ! categ_selected(categ));
				}

				std::pair<size_type, size_type> last() const
				{
					container::value_type const & catobj = *list_.rbegin();
					size_type n = catobj.items.size();
					size_type cat = list_.size() - 1;
					if(cat == 0)
					{
						if(n)	--n;
					}
					else
					{
						if(n && catobj.expand)
							--n;
						else
							n = npos;
					}
					return std::pair<size_type, size_type>(cat, n);
				}

				bool good(size_type cat) const
				{
					return (cat < list_.size());
				}

				bool good(size_type cat, size_type index) const
				{
					if(cat < list_.size())
						return (index < size_item(cat));
					return false;
				}

				bool good_item(size_type categ, size_type index, std::pair<size_type, size_type>& item) const
				{
					if(categ == 0 && index == npos)	index = 0;

					if(categ < list_.size())
					{
						if(index != npos)
						{
							container::const_iterator i = list_.begin();
							std::advance(i, categ);

							if(index >= i->items.size())
							{
								if(++i != list_.end())
								{
									++categ;
									index = npos;
								}
								else
									return false;
							}
						}

						item.first = categ;
						item.second = index;
						return true;
					}
					return false;
				}

				//Translate relative position into absolute position
				size_type absolute(size_type cat, size_type index) const
				{
					return (sorted_index_ == npos ? index : _m_at(cat)->sorted[index]);
				}

				bool forward(size_type cat, size_type index, size_type offs, std::pair<size_type, size_type>& item) const
				{
					std::pair<size_type, size_type> good;
					if(good_item(cat, index, good) == false)
						return false;

					cat = good.first;
					index = good.second;

					if(offs == 0)
					{
						item.first = cat;
						item.second = index;
						return true;
					}

					if(list_.size() <= cat) return false;

					//this is a category, so...
					if(npos == index)
					{
						//because the first is a category, and offs must not be 0, the category would not be candidated.
						//the algorithm above to calc the offset item is always starting with a item.
						--offs;
						index = 0;
					}

					container::const_iterator icat = list_.begin();
					std::advance(icat, cat);

					if(icat->items.size() <= index) return false;

					if(icat->expand)
					{
						std::size_t item_size = icat->items.size() - index;
						if(offs < item_size)
						{
							item.first = cat;
							item.second = offs + index;
							return true;
						}
						else
							offs -= item_size;
					}

					++cat;
					++icat;
					for(; icat != list_.end(); ++icat, ++cat)
					{
						if(offs-- == 0)
						{
							item.first = cat;
							item.second = npos;
							return true;
						}

						if(icat->expand)
						{
							if(offs < icat->items.size())
							{
								item.first = cat;
								item.second = offs;
								return true;
							}
							else
								offs -= icat->items.size();
						}
					}
					return false;
				}

				bool backward(size_type categ, size_type index, size_type offs, std::pair<size_type, size_type>& item) const
				{
					if(offs == 0)
					{
						item.first = categ;
						item.second = index;
					}

					if(categ < list_.size())
					{
						container::const_iterator i = list_.begin();
						std::advance(i, categ);

						size_type n = (index == npos ? 1 : index + 2);
						if(n <= offs)
						{
							offs -= n;
						}
						else
						{
							n -=offs;
							item.first = categ;
							item.second = (n == 1 ? npos : n - 2);
							return true;
						}

						while(i != list_.begin())
						{
							--i;
							--categ;

							n = (i->expand ? i->items.size() : 0) + 1;

							if(n > offs)
							{
								n -=offs;
								item.first = categ;
								item.second = (n == 1 ? npos : n - 2);
								return true;
							}
							else
								offs -= n;
						}
					}
					return false;
				}
			private:
				container::iterator _m_at(size_type index)
				{
					if(index >= list_.size())
						throw std::out_of_range("Nana.GUI.Listbox: invalid category index");

					container::iterator i = list_.begin();
					std::advance(i, index);
					return i;
				}

				const container::const_iterator _m_at(size_type index) const
				{
					if(index >= list_.size())
						throw std::out_of_range("Nana.GUI.Listbox: invalid category index");

					container::const_iterator i = list_.begin();
					std::advance(i, index);
					return i;
				}
			private:
				nana::gui::listbox * widget_;
				std::size_t sorted_index_;	//It stands for the index of header which is used to sort.
				bool	sorted_reverse_;
				container list_;
			};//end class es_lister

			//struct essence_t
			//@brief:	this struct gives many data for listbox,
			//			the state of the struct does not effect on member funcions, therefore all data members are public.
			struct essence_t
			{
				typedef std::size_t size_type;

				struct state
				{
					enum t{normal, highlighted, pressed, grabed, floated};
				};

				struct where
				{
					enum t{unknown = -1, header, lister, checker};
				};

				nana::paint::graphics *graph;
				bool auto_draw;
				bool checkable;
				bool if_image;
				unsigned header_size;
				unsigned item_size;
				unsigned text_height;
				unsigned suspension_width;

				es_header header;
				es_lister lister;
				nana::any resolver;

				state::t ptr_state;
				std::pair<where::t, size_type> pointer_where;	//'first' stands for which object, such as header and lister, 'second' stands for item
																//if where == header, 'second' indicates the item
																//if where == lister || where == checker, 'second' indicates the offset to the scroll offset_y which stands for the first item displayed in lister.
																//if where == unknown, 'second' ignored.

				struct
				{
					static const unsigned scale = 16;
					int offset_x;
					nana::upoint offset_y;	//x stands for category, y stands for item. "y == npos" means that is a category.

					nana::gui::scroll<true> v;
					nana::gui::scroll<false> h;
				}scroll;

				essence_t()
					:	graph(0), auto_draw(true), checkable(false), if_image(false),
						header_size(25), item_size(24), text_height(0), suspension_width(0),
						ptr_state(state::normal)
				{
					scroll.offset_x = 0;
					pointer_where.first = where::unknown;
					lister.fetch_ordering_comparer = nana::make_fun(header, &es_header::fetch_comp);
				}

				nana::upoint scroll_y() const
				{
					return scroll.offset_y;
				}

				void scroll_y(const nana::upoint& pos)
				{
					if(pos.x < lister.size_categ())
					{
						scroll.offset_y.x = pos.x;
						size_type number = lister.size_item(pos.x);
						if(pos.y < number)
							scroll.offset_y.y = pos.y;
						else if(number)
							scroll.offset_y.y = static_cast<nana::upoint::value_type>(number - 1);
						else
							scroll.offset_y.y = (pos.x > 0 ? where::unknown : 0);
					}
				}

				//number_of_lister_items
				//@brief: Returns the number of items that are contained in pixels
				//@param,with_rest: Means whether including extra one item that is not completely contained in reset pixels.
				size_type number_of_lister_items(bool with_rest) const
				{
					unsigned lister_s = graph->height() - 2 - (header.visible() ? header_size: 0) - (scroll.h.empty() ? 0 : scroll.scale);
					return (lister_s / item_size) + (with_rest && (lister_s % item_size) ? 1 : 0);
				}

				//keep the first selected item in the display area
				void trace_selected_item()
				{
					std::vector<std::pair<size_type, size_type> > svec;
					lister.item_selected(svec);
					if(0 == svec.size()) return;	//no selected, exit.

					std::pair<size_type, size_type> & item = svec[0];
					//Same with current scroll offset item.
					if(item.second == npos && item.first == scroll.offset_y.x && (scroll.offset_y.y == npos))
						return;

					if(item.first < scroll.offset_y.x || (item.first == scroll.offset_y.x && (scroll.offset_y.y != npos) && (item.second == npos || item.second < scroll.offset_y.y)))
					{
						scroll.offset_y.x = static_cast<nana::upoint::value_type>(item.first);
						scroll.offset_y.y = static_cast<nana::upoint::value_type>(item.second);
						if(lister.expand(item.first) == false)
						{
							if(lister.categ_selected(item.first))
								scroll.offset_y.y = where::unknown;
							else
								lister.expand(item.first, true);
						}
					}
					else
					{
						size_type numbers = number_of_lister_items(false);
						size_type off = lister.distance(scroll.offset_y.x, scroll.offset_y.y, item.first, item.second);
						if(numbers > off) return;
						std::pair<size_type, size_type> n_off = lister.advance(scroll.offset_y.x, scroll.offset_y.y, (off - numbers) + 1);
						if(n_off.first != npos)
						{
							scroll.offset_y.x = static_cast<nana::upoint::value_type>(n_off.first);
							scroll.offset_y.y = static_cast<nana::upoint::value_type>(n_off.second);
						}
					}

					adjust_scroll_life();
					adjust_scroll_value();
				}

				void adjust_scroll_value()
				{
					if(scroll.h.empty() == false)
					{
						unsigned width = 4 + (scroll.v.empty() ? 0 : scroll.scale - 1);
						if(width >= graph->width()) return;
						scroll.h.amount(header.pixels());
						scroll.h.range(graph->width() - width);
						scroll.h.value(scroll.offset_x);
					}

					if(scroll.v.empty() == false)
					{
						unsigned height = 2 + (scroll.h.empty() ? 0 : scroll.scale);
						if(height >= graph->width()) return;
						scroll.v.amount(lister.the_number_of_expanded());
						scroll.v.range(number_of_lister_items(false));
						size_type off = lister.distance(0, 0, scroll.offset_y.x, scroll.offset_y.y);
						scroll.v.value(off);
					}
				}

				void adjust_scroll_life()
				{
					const nana::size sz = graph->size();
					unsigned header_s = header.pixels();
					window wd = lister.wd_ptr()->handle();

					//H scroll enabled
					bool h = (header_s > sz.width - 4);

					unsigned lister_s = sz.height - 2 - (header.visible() ? header_size: 0) - (h ? scroll.scale : 0);
					size_type screen_number = (lister_s / item_size);

					//V scroll enabled
					bool v = (lister.the_number_of_expanded() > screen_number);

					if(v == true && h == false)
						h = (header_s > (sz.width - 2 - scroll.scale));

					unsigned width = sz.width - 2 - (v ? scroll.scale : 0);
					unsigned height = sz.height - 2 - (h ? scroll.scale : 0);

					if(h)
					{
						rectangle r(1, sz.height - scroll.scale - 1, width, scroll.scale);
						if(scroll.h.empty())
						{
							scroll.h.create(wd, r);
							API::take_active(scroll.h.handle(), false, wd);
							scroll.h.make_event<events::mouse_move>(*this, &essence_t::_m_answer_scroll);
							scroll.h.make_event<events::mouse_up>(*this, &essence_t::_m_answer_scroll);
						}
						else
							scroll.h.move(r.x, r.y, r.width, r.height);
					}
					else if(!scroll.h.empty())
						scroll.h.close();

					if(v)
					{
						rectangle r(sz.width - 1 - scroll.scale, 1, scroll.scale, height);
						if(scroll.v.empty())
						{
							scroll.v.create(wd, r);
							API::take_active(scroll.v.handle(), false, wd);
							scroll.v.make_event<events::mouse_move>(*this, &essence_t::_m_answer_scroll);
							scroll.v.make_event<events::mouse_up>(*this, &essence_t::_m_answer_scroll);
						}
						else
							scroll.v.move(r.x, r.y, r.width, r.height);
					}
					else if(!scroll.v.empty())
					{
						scroll.v.close();
						scroll.offset_y.x = scroll.offset_y.y = 0;

						nana::rectangle r;
						if(rect_header(r))
						{
							if(header_s > r.width)
							{
								if((header_s - scroll.offset_x) < r.width)
									scroll.offset_x = header_s - r.width;
							}
							else
								scroll.offset_x = 0;
						}
					}
					adjust_scroll_value();
				}

				void set_auto_draw(bool ad)
				{
					if(auto_draw != ad)
					{
						auto_draw = ad;
						if(ad)
						{
							adjust_scroll_life();
							API::refresh_window(lister.wd_ptr()->handle());
						}
					}
				}

				nana::rectangle checkarea(int x, int y) const
				{
					return nana::rectangle(x + 4, y + (item_size - 16) / 2, 16, 16);
				}

				bool is_checkarea(const nana::point& item_pos, const nana::point& mspos) const
				{
					nana::rectangle r = checkarea(item_pos.x, item_pos.y);
					return ((r.x <= mspos.x && mspos.x <= r.x + static_cast<int>(r.width)) && (r.y <= mspos.y && mspos.y < r.y + static_cast<int>(r.height)));
				}

				int item_xpos(const nana::rectangle& r) const
				{
					std::vector<es_header::size_type> seq;
					header_seq(seq, r.width);
					return (seq.size() ? (header.xpos(seq[0]) - scroll.offset_x + r.x) : 0);
				}

				bool calc_where(int x, int y)
				{
					std::pair<where::t, size_type> new_where;

					if(2 < x && x < static_cast<int>(graph->width()) - 2 && 1 < y && y < static_cast<int>(graph->height()) - 1)
					{
						if(header.visible() && y < static_cast<int>(header_size + 1))
						{
							x -= (2 - scroll.offset_x);
							new_where.first = where::header;
							new_where.second = static_cast<int>(header.item_by_x(x));
						}
						else
						{
							new_where.second = (y - (header.visible() ? header_size : 0) + 1) / item_size;
							new_where.first = where::lister;
							if(checkable)
							{
								nana::rectangle r;
								if(rect_lister(r))
								{
									std::size_t top = new_where.second * item_size + (header.visible() ? header_size : 0);
									if(is_checkarea(nana::point(item_xpos(r), static_cast<int>(top)), nana::point(x, y)))
										new_where.first = where::checker;
								}
							}
						}
					}
					else
					{
						new_where.first = where::unknown;
						new_where.second = npos;
					}

					if(new_where != pointer_where)
					{
						pointer_where = new_where;
						return true;
					}
					return false;
				}

				void widget_to_header(nana::point& pos)
				{
					--pos.y;
					pos.x += (scroll.offset_x - 2);
				}

				bool rect_header(nana::rectangle& r) const
				{
					if(header.visible())
					{
						const unsigned ex_width = 4 + (scroll.v.empty() ? 0 : scroll.scale - 1);
						if(graph->width() > ex_width)
						{
							r.x = 2;
							r.y = 1;
							r.width = graph->width() - ex_width;
							r.height = header_size;
							return true;
						}
					}
					return false;
				}

				bool rect_lister(nana::rectangle& r) const
				{
					unsigned width = 4 + (scroll.v.empty() ? 0 : scroll.scale - 1);
					unsigned height = 2 + (scroll.h.empty() ? 0 : scroll.scale) + (header.visible() ? header_size : 0);

					if(graph->width() <= width || graph->height() <= height) return false;

					r.x = 2;
					r.y = (header.visible() ? header_size + 1 : 1);
					r.width = graph->width() - width;
					r.height = graph->height() - height;
					return true;
				}

				bool wheel(bool upwards)
				{
					std::pair<size_type, size_type> target;

					if(upwards == false)
						lister.forward(scroll.offset_y.x, scroll.offset_y.y, 1, target);
					else
						lister.backward(scroll.offset_y.x, scroll.offset_y.y, 1, target);

					if(target.first != scroll.offset_y.x || target.second != scroll.offset_y.y)
					{
						scroll.offset_y.x = static_cast<nana::upoint::value_type>(target.first);
						scroll.offset_y.y = static_cast<nana::upoint::value_type>(target.second);
						return true;
					}
					return false;
				}

				void header_seq(std::vector<es_header::size_type> &seqs, unsigned lister_w)const
				{
					int x = - (scroll.offset_x);
					for(std::vector<es_header::item_t>::const_iterator i = header.cont().begin(); i != header.cont().end(); ++i)
					{
						if(false == i->visible) continue;
						x += i->pixels;
						if(x > 0)
							seqs.push_back(i->index);
						if(x >= static_cast<int>(lister_w))
							break;
					}
				}
			private:
				void _m_answer_scroll(const eventinfo& ei)
				{
					if(ei.identifier == events::mouse_move::identifier && ei.mouse.left_button == false) return;

					bool update = false;
					if(ei.window == scroll.v.handle())
					{
						std::pair<size_type, size_type> item;
						if(lister.forward(0, 0, scroll.v.value(), item))
						{
							if(item.second != scroll.offset_y.y || item.first != scroll.offset_y.x)
							{
								scroll.offset_y.x = static_cast<nana::upoint::value_type>(item.first);
								scroll.offset_y.y = static_cast<nana::upoint::value_type>(item.second);
								update = true;
							}
						}
					}
					else if(ei.window == scroll.h.handle())
					{
						if(scroll.offset_x != static_cast<int>(scroll.h.value()))
						{
							scroll.offset_x = static_cast<int>(scroll.h.value());
							update = true;
						}
					}

					if(update)
						API::refresh_window(lister.wd_ptr()->handle());
				}
			};

			class drawer_header_impl
			{
			public:
				typedef es_header::size_type size_type;
				typedef nana::paint::graphics & graph_reference;

				drawer_header_impl(essence_t* es): item_spliter_(npos), essence_(es){}

				size_type item_spliter() const
				{
					return item_spliter_;
				}

				void cancel_spliter()
				{
					item_spliter_ = npos;
				}

				bool mouse_spliter(const nana::rectangle& r, int x)
				{
					if(essence_->ptr_state == essence_t::state::highlighted)
					{
						x -= (r.x - essence_->scroll.offset_x);

						for(es_header::container::const_iterator i = essence_->header.cont().begin(); i != essence_->header.cont().end(); ++i)
						{
							if(i->visible)
							{
								if((static_cast<int>(i->pixels)  - 2 < x) && (x < static_cast<int>(i->pixels) + 3))
								{
									item_spliter_ = i->index;
									return true;
								}
								x -= i->pixels;
							}
						}
					}
					else if(essence_->ptr_state == essence_t::state::normal)
						item_spliter_ = npos;
					return false;
				}

				void grab(const nana::point& pos, bool is_grab)
				{
					if(is_grab)
					{
						ref_xpos_ = pos.x;
						if(item_spliter_ != npos)
							orig_item_width_ = essence_->header.get_item(item_spliter_).pixels;
					}
					else if(grab_terminal_.index != npos && grab_terminal_.index != essence_->pointer_where.second)
						essence_->header.move(essence_->pointer_where.second, grab_terminal_.index, grab_terminal_.place_front);
				}

				//grab_move
				//@brief: draw when an item is grabbing.
				//@return: 0 = no graphics changed, 1 = just update, 2 = refresh
				int grab_move(const nana::rectangle& rect, const nana::point& pos)
				{
					if(item_spliter_ == npos)
					{
						draw(rect);
						_m_make_float(rect, pos);

						//Draw the target strip
						grab_terminal_.index = _m_target_strip(pos.x, rect, essence_->pointer_where.second, grab_terminal_.place_front);
						return 1;
					}
					else
					{
						const es_header::item_t& item = essence_->header.get_item(item_spliter_);
						//Resize the item specified by item_spliter_.
						int new_w = orig_item_width_ - (ref_xpos_ - pos.x);
						if(static_cast<int>(item.pixels) != new_w)
						{
							essence_->header.item_width(item_spliter_, (new_w < static_cast<int>(essence_->suspension_width + 20) ? essence_->suspension_width + 20 : new_w));
							unsigned new_w = essence_->header.pixels();
							if(new_w < rect.width + essence_->scroll.offset_x)
							{
								essence_->scroll.offset_x = (new_w > rect.width ? new_w - rect.width : 0);
							}
							essence_->adjust_scroll_life();
							return 2;
						}
					}
					return 0;
				}

				void draw(const nana::rectangle& r)
				{
					_m_draw(essence_->header.cont(), r);

					const int y = r.y + r.height - 1;
					essence_->graph->line(r.x, y, r.x + r.width, y, 0xDEDFE1);
				}
			private:
				size_type _m_target_strip(int x, const nana::rectangle& rect, size_type grab, bool& place_front)
				{
					//convert x to header logic coordinate.
					if(x < essence_->scroll.offset_x)
						x = essence_->scroll.offset_x;
					else if(x > essence_->scroll.offset_x + static_cast<int>(rect.width))
						x = essence_->scroll.offset_x + rect.width;

					size_type i = essence_->header.item_by_x(x);
					if(i == npos)
					{
						i = (essence_->header.xpos(grab) < x ? essence_->header.last() : essence_->header.begin());
					}
					if(grab != i)
					{
						int item_xpos;
						unsigned item_pixels;
						if(essence_->header.item_pos(i, item_xpos, item_pixels))
						{
							int midpos = item_xpos + static_cast<int>(item_pixels / 2);

							//Get the item pos
							//if mouse pos is at left of an item middle, the pos of itself otherwise the pos of the next.
							place_front = (x <= midpos);
							x = (place_front ? item_xpos : essence_->header.xpos(essence_->header.neighbor(i, false)));

							if(i != npos)
								essence_->graph->rectangle(x - essence_->scroll.offset_x + rect.x, rect.y, 2, rect.height, 0xFF0000, true);
						}
						return i;
					}
					return npos;

				}

				template<typename Container>
				void _m_draw(const Container& cont, const nana::rectangle& rect)
				{
					graph_reference graph = *(essence_->graph);
					int x = rect.x - essence_->scroll.offset_x;
					unsigned height = rect.height - 1;

					int txtop = (rect.height - essence_->text_height) / 2 + rect.y;

					nana::color_t txtcolor = essence_->lister.wd_ptr()->foreground();

					essence_t::state::t state = essence_t::state::normal;
					//check whether grabing an item, if item_spliter_ != npos, that indicates the grab item is a spliter.
					if(essence_->pointer_where.first == essence_t::where::header && (item_spliter_ == npos))
						state = essence_->ptr_state;

					int bottom_y = static_cast<int>(rect.y + rect.height - 2);
					for(typename Container::const_iterator i = cont.begin(); i != cont.end(); ++i)
					{
						if(i->visible)
						{
							if(x + static_cast<int>(i->pixels) > rect.x)
							{
								_m_draw_item(graph, x, rect.y, height, txtop, txtcolor, *i, (i->index == essence_->pointer_where.second ? state : essence_t::state::normal));

								essence_->graph->line(x - 1 + i->pixels, rect.y, x - 1 + i->pixels, bottom_y, 0xDEDFE1);
							}
							x += i->pixels;

							if(x - rect.x > static_cast<int>(rect.width)) break;
						}
					}

					if(x - rect.x < static_cast<int>(rect.width))
						essence_->graph->rectangle(x, rect.y, rect.width - x + rect.x, height, 0xF1F2F4, true);
				}

				template<typename Item>
				void _m_draw_item(graph_reference graph, int x, int y, unsigned height, int txtop, nana::color_t txtcolor, const Item& item, int state)
				{
					nana::color_t bkcolor;
					typedef essence_t::state state_t;
					switch(state)
					{
					case state_t::normal:	bkcolor = 0xF1F2F4; break;
					case state_t::highlighted:	bkcolor = 0xFFFFFF; break;
					case state_t::pressed:
					case state_t::grabed:		bkcolor = 0x8BD6F6; break;
					case state_t::floated:		bkcolor = 0xBABBBC; break;
					}

					graph.rectangle(x, y, item.pixels, height, bkcolor, true);
					graph.string(x + 5, txtop, txtcolor, item.text);
					if(item.index == essence_->lister.sort_index())
					{
						nana::paint::gadget::directions::t dir = essence_->lister.sort_reverse() ? nana::paint::gadget::directions::to_south : nana::paint::gadget::directions::to_north;
						nana::paint::gadget::arrow_16_pixels(graph, x + (item.pixels - 16) / 2, -4, 0x0, 0, dir);
					}
				}

				void _m_make_float(const nana::rectangle& rect, const nana::point& pos)
				{
					const es_header::item_t & item = essence_->header.get_item(essence_->pointer_where.second);

					nana::paint::graphics ext_graph(item.pixels, essence_->header_size);
					ext_graph.typeface(essence_->graph->typeface());

					int txtop = (essence_->header_size - essence_->text_height) / 2;
					_m_draw_item(ext_graph, 0, 0, essence_->header_size, txtop, 0xFFFFFF, item, essence_t::state::floated);

					int xpos = essence_->header.xpos(item.index) + pos.x - ref_xpos_;
					ext_graph.blend(*(essence_->graph), xpos - essence_->scroll.offset_x + rect.x, rect.y, 0.5);
				}
			private:
				int			ref_xpos_;
				unsigned	orig_item_width_;
				size_type	item_spliter_;
				struct grab_terminal
				{
					size_type index;
					bool place_front;
				}grab_terminal_;
				essence_t * essence_;
			};

			class drawer_lister_impl
			{
			public:
				typedef es_lister::size_type size_type;

				drawer_lister_impl(essence_t * es): essence_(es){}

				void draw(const nana::rectangle& rect) const
				{
					typedef es_lister::container::value_type::container container;

					size_type n = essence_->number_of_lister_items(true);
					if(0 == n)return;

					widget * wdptr = essence_->lister.wd_ptr();
					nana::color_t bkcolor = wdptr->background();
					nana::color_t txtcolor = wdptr->foreground();

					unsigned header_w = essence_->header.pixels();
					if(header_w - essence_->scroll.offset_x < rect.width)
						essence_->graph->rectangle(rect.x + header_w - essence_->scroll.offset_x, rect.y, rect.width - (header_w - essence_->scroll.offset_x), rect.height, bkcolor, true);

					es_lister & lister = essence_->lister;
					//The Tracker indicates the item where mouse placed.
					std::pair<es_lister::size_type, es_lister::size_type> tracker(npos, npos);
					if((essence_->pointer_where.first == essence_t::where::lister || essence_->pointer_where.first == essence_t::where::checker) && essence_->pointer_where.second != npos)
						lister.forward(essence_->scroll.offset_y.x, essence_->scroll.offset_y.y, essence_->pointer_where.second, tracker);

					std::vector<es_header::size_type> subitems;
					essence_->header_seq(subitems, rect.width);

					if(subitems.size() == 0) return;

					int x = essence_->item_xpos(rect);
					int y = rect.y;
					int txtoff = (essence_->item_size - essence_->text_height) / 2;

					es_lister::container::const_iterator icat = essence_->lister.cat_container().begin();
					std::advance(icat, essence_->scroll.offset_y.x);

					size_type catg_idx = essence_->scroll.offset_y.x;
					size_type item_idx = essence_->scroll.offset_y.y;

					essence_t::state::t state = essence_t::state::normal;
					//Here draws a root categ or a first drawing is not a categ.
					if(catg_idx == 0 || item_idx != npos)
					{
						if(catg_idx == 0 && item_idx == npos)
						{
							essence_->scroll.offset_y.y = 0;
							item_idx = 0;
						}

						//Test whether the sort is enabled.
						if(essence_->lister.sort_index() != npos)
						{
							std::size_t size = icat->items.size();
							for(std::size_t offs = essence_->scroll.offset_y.y; offs < size; ++offs, ++item_idx)
							{
								if(n-- == 0)	break;
								state = (tracker.first == catg_idx && tracker.second == item_idx	?
									essence_t::state::highlighted : essence_t::state::normal);

								_m_draw_item(icat->items[lister.absolute(catg_idx, offs)], x, y, txtoff, header_w, rect, subitems, bkcolor, txtcolor, state);
								y += essence_->item_size;
							}
						}
						else
						{
							for(container::const_iterator i = icat->items.begin() + essence_->scroll.offset_y.y; i != icat->items.end(); ++i, ++item_idx)
							{
								if(n-- == 0) break;
								if(tracker.first == catg_idx && tracker.second == item_idx)
									state = essence_t::state::highlighted;
								else
									state = essence_t::state::normal;

								_m_draw_item(*i, x, y, txtoff, header_w, rect, subitems, bkcolor, txtcolor, state);
								y += essence_->item_size;
							}
						}
						++icat;
						++catg_idx;
					}

					for(; icat != essence_->lister.cat_container().end(); ++icat, ++catg_idx)
					{
						if(n-- == 0) break;
						item_idx = 0;

						if(tracker.second == npos && tracker.first == catg_idx)
							state = essence_t::state::highlighted;
						else
							state = essence_t::state::normal;

						_m_draw_categ(*icat, rect.x - essence_->scroll.offset_x, y, txtoff, header_w, rect, bkcolor, state);
						y += essence_->item_size;

						if(false == icat->expand) continue;

												//Test whether the sort is enabled.
						if(essence_->lister.sort_index() != npos)
						{
							std::size_t size = icat->items.size();
							for(std::size_t pos = 0; pos < size; ++pos)
							{
								if(n-- == 0)	break;
								state = (tracker.first == catg_idx && tracker.second == item_idx	?
									essence_t::state::highlighted : essence_t::state::normal);

								_m_draw_item(icat->items[lister.absolute(catg_idx, pos)], x, y, txtoff, header_w, rect, subitems, bkcolor, txtcolor, state);
								y += essence_->item_size;
								++item_idx;
							}						
						}
						else
						{
							for(container::const_iterator i = icat->items.begin(); i != icat->items.end(); ++i, ++item_idx)
							{
								if(n-- == 0) break;

								if(tracker.first == catg_idx && tracker.second == item_idx)
									state = essence_t::state::highlighted;
								else
									state = essence_t::state::normal;

								_m_draw_item(*i, x, y, txtoff, header_w, rect, subitems, bkcolor, txtcolor, state);
								y += essence_->item_size;
							}
						}
					}

					if(y < rect.y + static_cast<int>(rect.height))
						essence_->graph->rectangle(rect.x, y, rect.width, rect.y + rect.height - y, bkcolor, true);
				}
			private:
				void _m_draw_categ(const es_lister::category& categ, int x, int y, int txtoff, unsigned width, const nana::rectangle& r, nana::color_t bkcolor, int state) const
				{
					bool sel = categ.select();
					if(sel && (categ.expand == false))
						bkcolor = 0xD5EFFC;

					if(state == essence_t::state::highlighted)
						bkcolor = essence_->graph->mix(bkcolor, 0x99DEFD, 0.8);

					essence_->graph->rectangle(x, y, width, essence_->item_size, bkcolor, true);

					nana::paint::gadget::arrow_16_pixels(*(essence_->graph), x + 5, y + (essence_->item_size - 16) /2, 0x3399, 2, (categ.expand ? nana::paint::gadget::directions::to_north : nana::paint::gadget::directions::to_south));
					nana::size text_s = essence_->graph->text_extent_size(categ.text);
					essence_->graph->string(x + 20, y + txtoff, 0x3399, categ.text);

					std::stringstream ss;
					ss<<'('<<static_cast<unsigned>(categ.items.size())<<')';
					nana::string str = nana::charset(ss.str());

					unsigned str_w = essence_->graph->text_extent_size(str).width;

					essence_->graph->string(x + 25 + text_s.width, y + txtoff, 0x3399, str);

					if(x + 35 + text_s.width + str_w < x + width)
						essence_->graph->line(x + 30 + text_s.width + str_w, y + essence_->item_size / 2, x + width - 5, y + essence_->item_size / 2, 0x3399);

					//Draw selecting inner rectangle
					if(sel && categ.expand == false)
					{
						width -= essence_->scroll.offset_x;
						_m_draw_border(r.x, y, (r.width < width ? r.width : width));
					}
				}

				void _m_draw_item(const es_lister::item_t& item, int x, int y, int txtoff, unsigned width, const nana::rectangle& r, const std::vector<size_type>& seqs, nana::color_t bkcolor, nana::color_t txtcolor, essence_t::state::t state) const
				{
					if(item.flags.selected)
						bkcolor = 0xD5EFFC;
					else if((item.bkcolor & 0xFF000000) == 0)
						bkcolor = item.bkcolor;

					if((item.fgcolor & 0xFF000000) == 0)
						txtcolor = item.fgcolor;

					if(state == essence_t::state::highlighted)
					{
						bkcolor = essence_->graph->mix(bkcolor, 0x99DEFD, 0.8);
					}

					unsigned show_w = width - essence_->scroll.offset_x;
					if(show_w >= r.width) show_w = r.width;

					//draw the background
					essence_->graph->rectangle(r.x, y, show_w, essence_->item_size, bkcolor, true);

					int img_off = (essence_->if_image ? (essence_->item_size - 16) / 2 : 0);
					
					int item_xpos = x;
					bool first = true;
					for(std::vector<size_type>::const_iterator i = seqs.begin(); i != seqs.end(); ++i)
					{
						size_type index = *i;

						const es_header::item_t & header = essence_->header.get_item(index);

						if((item.texts.size() > index) && (header.pixels > 5))
						{
							int ext_w = 0;
							if(first && essence_->checkable)
							{
								ext_w = 18;
								nana::rectangle chkarea = essence_->checkarea(item_xpos, y);

								mouse_action::t act = mouse_action::normal;

								if(essence_->pointer_where.first == essence_t::where::checker)
								{
									switch(state)
									{
									case essence_t::state::normal:
										break;
									case essence_t::state::highlighted:
										act = mouse_action::over;
										break;
									case essence_t::state::grabed:
										act = mouse_action::pressed;
										break;
									default:	break;
									}
								}

								chk_renderer_.render(*essence_->graph, chkarea.x, chkarea.y, chkarea.width, chkarea.height, act, chk_renderer_.clasp, item.flags.checked);
							}
							nana::size ts = essence_->graph->text_extent_size(item.texts[index]);

							if((0 == index) && essence_->if_image)
							{
								ext_w += 18;
								if(item.img)
									item.img.stretch(nana::rectangle(), *essence_->graph, nana::rectangle(item_xpos + 5, y + img_off, 16, 16));
							}
							essence_->graph->string(item_xpos + 5 + ext_w, y + txtoff, txtcolor, item.texts[index]);

							if(ts.width + 5 + ext_w > header.pixels)
							{//The text is painted over the next subitem
								int xpos = item_xpos + header.pixels - essence_->suspension_width;
								essence_->graph->rectangle(xpos, y + 2, ts.width + 5 + ext_w - header.pixels + essence_->suspension_width, essence_->item_size - 4, bkcolor, true);
								essence_->graph->string(xpos, y + 2, txtcolor, STR("..."));
							}
						}

						essence_->graph->line(item_xpos - 1, y, item_xpos - 1, y + essence_->item_size - 1, 0xEBF4F9);

						item_xpos += header.pixels;
						first = false;
					}

					//Draw selecting inner rectangle
					if(item.flags.selected)
						_m_draw_border(r.x, y, show_w);
				}
				void _m_draw_border(int x, int y, unsigned width) const
				{
					//Draw selecting inner rectangle
					nana::paint::graphics * graph = essence_->graph;
					graph->rectangle(x , y , width, essence_->item_size, 0x99DEFD, false);

					graph->rectangle(x + 1, y + 1, width - 2, essence_->item_size - 2, 0xFFFFFF, false);
					graph->set_pixel(x, y, 0xFFFFFF);
					graph->set_pixel(x, y + essence_->item_size - 1, 0xFFFFFF);
					graph->set_pixel(x + width - 1, y, 0xFFFFFF);
					graph->set_pixel(x + width - 1, y + essence_->item_size - 1, 0xFFFFFF);
				}
			private:
				essence_t * essence_;
				mutable nana::paint::gadget::check_renderer chk_renderer_;
			};

			//class trigger: public drawer_trigger
				trigger::trigger()
					: essence_(new essence_t),
						drawer_header_(new drawer_header_impl(essence_)),
						drawer_lister_(new drawer_lister_impl(essence_))
				{}

				trigger::~trigger()
				{
					delete drawer_lister_;
					delete drawer_header_;
					delete essence_;
				}

				essence_t& trigger::essence()
				{
					return *essence_;
				}

				essence_t& trigger::essence() const
				{
					return *essence_;
				}

				void trigger::draw()
				{
					nana::rectangle rect;

					if(essence_->header.visible())
					{
						if(essence_->rect_header(rect))
							drawer_header_->draw(rect);
					}
					if(essence_->rect_lister(rect))
						drawer_lister_->draw(rect);
					_m_draw_border();
				}

				void trigger::update()
				{
					if(essence_->auto_draw)
					{
						essence_->adjust_scroll_life();
						API::refresh_window(*(essence_->lister.wd_ptr()));
					}
				}

				void trigger::_m_draw_border()
				{
					nana::paint::graphics * graph = essence_->graph;
					//Draw Border
					graph->rectangle(0, 0, graph->width(), graph->height(), 0x9CB6C5, false);
					graph->line(1, 1, 1, graph->height() - 2, 0xFFFFFF);
					graph->line(graph->width() - 2, 1, graph->width() - 2, graph->height() - 2, 0xFFFFFF);

					if((essence_->scroll.h.empty() == false) && (essence_->scroll.v.empty() == false))
						graph->rectangle(graph->width() - 1 - essence_->scroll.scale, graph->height() - 1 - essence_->scroll.scale, essence_->scroll.scale, essence_->scroll.scale, nana::gui::color::button_face, true);
				}

				void trigger::bind_window(widget_reference wd)
				{
					essence_->lister.bind(wd);
					wd.background(0xFFFFFF);
				}

				void trigger::attached(graph_reference graph)
				{
					essence_->graph = &graph;
					typeface_changed(graph);

					window wd = essence_->lister.wd_ptr()->handle();
					using namespace API::dev;
					make_drawer_event<events::mouse_move>(wd);
					make_drawer_event<events::mouse_leave>(wd);
					make_drawer_event<events::mouse_down>(wd);
					make_drawer_event<events::mouse_up>(wd);
					make_drawer_event<events::dbl_click>(wd);
					make_drawer_event<events::size>(wd);
					make_drawer_event<events::mouse_wheel>(wd);
					make_drawer_event<events::key_down>(wd);
				}

				void trigger::typeface_changed(graph_reference graph)
				{
					essence_->text_height = graph.text_extent_size(STR("jHWn0123456789/<?'{[|\\_")).height;
					essence_->item_size = essence_->text_height + 6;
					essence_->suspension_width = graph.text_extent_size(STR("...")).width;				
				}

				void trigger::detached()
				{
					essence_->graph = 0;
					API::dev::umake_drawer_event(essence_->lister.wd_ptr()->handle());
				}

				void trigger::refresh(graph_reference)
				{
					draw();
				}

				void trigger::mouse_move(graph_reference graph, const eventinfo& ei)
				{
					int update = 0; //0 = nothing, 1 = update, 2 = refresh
					if(essence_->ptr_state == essence_t::state::pressed)
					{
						if(essence_->pointer_where.first == essence_t::where::header)
						{
							essence_->ptr_state = essence_t::state::grabed;
							nana::point pos(ei.mouse.x, ei.mouse.y);
							essence_->widget_to_header(pos);
							drawer_header_->grab(pos, true);
							API::capture_window(essence_->lister.wd_ptr()->handle(), true);
							update = 2;
						}
					}

					if(essence_->ptr_state == essence_t::state::grabed)
					{
						nana::point pos(ei.mouse.x, ei.mouse.y);
						essence_->widget_to_header(pos);

						nana::rectangle r;
						essence_->rect_header(r);
						update = drawer_header_->grab_move(r, pos);
					}
					else if(essence_->calc_where(ei.mouse.x, ei.mouse.y))
					{
						essence_->ptr_state = essence_t::state::highlighted;
						update = 2;
					}

					bool set_spliter = false;
					if(essence_->pointer_where.first == essence_t::where::header)
					{
						nana::rectangle r;
						if(essence_->rect_header(r))
						{
							if(drawer_header_->mouse_spliter(r, ei.mouse.x))
							{
								set_spliter = true;
								essence_->lister.wd_ptr()->cursor(nana::gui::cursor::size_we);
							}
						}
					}
					if((set_spliter == false) && (essence_->ptr_state != essence_t::state::grabed))
					{
						if((drawer_header_->item_spliter() != npos) || (essence_->lister.wd_ptr()->cursor() == nana::gui::cursor::size_we))
						{
							essence_->lister.wd_ptr()->cursor(nana::gui::cursor::arrow);
							drawer_header_->cancel_spliter();
							update = 2;
						}
					}

					switch(update)
					{
					case 1:
						API::update_window(essence_->lister.wd_ptr()->handle());
						break;
					case 2:
						draw();
						API::lazy_refresh();
						break;
					}
				}

				void trigger::mouse_leave(graph_reference graph, const eventinfo&)
				{
					if((essence_->pointer_where.first != essence_t::where::unknown) || (essence_->ptr_state != essence_t::state::normal))
					{
						if(essence_->ptr_state != essence_t::state::grabed)
						{
							essence_->pointer_where.first = essence_t::where::unknown;
							essence_->ptr_state = essence_t::state::normal;
						}
						draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_down(graph_reference graph, const eventinfo& ei)
				{
					bool update = false;
					if((essence_->pointer_where.first == essence_t::where::header) && ((essence_->pointer_where.second != npos) || (drawer_header_->item_spliter() != npos)))
					{
						essence_->ptr_state = essence_t::state::pressed;
						nana::rectangle r;
						if(essence_->rect_header(r))
						{
							drawer_header_->draw(r);
							update = true;
						}
					}
					else if(essence_->pointer_where.first == essence_t::where::lister || essence_->pointer_where.first == essence_t::where::checker)
					{
						es_lister & lister = essence_->lister;
						std::pair<size_type, size_type> item;
						if(lister.forward(essence_->scroll.offset_y.x, essence_->scroll.offset_y.y, essence_->pointer_where.second, item))
						{
							es_lister::item_t * item_ptr = (item.second != npos ? &lister.at(item.first, item.second) : 0);
							if(essence_->pointer_where.first == essence_t::where::lister)
							{
								lister.select_for_all(false);
								if(item_ptr)
								{
									item_ptr->flags.selected = true;
									essence_->lister.ext_event.selected(*essence_->lister.wd_ptr(), item.first, lister.absolute(item.first, item.second), true);
								}
								else
									lister.categ_selected(item.first, true);
							}
							else
							{
								if(item_ptr)
								{
									item_ptr->flags.checked = ! item_ptr->flags.checked;
									essence_->lister.ext_event.checked(*essence_->lister.wd_ptr(), item.first, lister.absolute(item.first, item.second), item_ptr->flags.checked);
								}
								else
									lister.categ_checked_reverse(item.first);
							}
							update = true;
						}
						else
							update = lister.select_for_all(false);	//unselect all items due to the blank area being clicked.

						if(update)
						{
							nana::rectangle r;
							update = essence_->rect_lister(r);
							if(update)
								drawer_lister_->draw(r);
						}
					}

					if(update)
					{
						_m_draw_border();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference graph, const eventinfo& ei)
				{
					essence_t::state::t prev_state = essence_->ptr_state;
					essence_->ptr_state = essence_t::state::highlighted;
					//Do sort
					if(essence_->pointer_where.first == essence_t::where::header && prev_state == essence_t::state::pressed)
					{
						if(essence_->pointer_where.second < essence_->header.cont().size())
						{
							if(essence_->lister.sort_index(essence_->pointer_where.second))
							{
								draw();
								API::lazy_refresh();
							}
						}
					}
					else if(prev_state == essence_t::state::grabed)
					{
						essence_->ptr_state = essence_t::state::highlighted;
						nana::point pos(ei.mouse.x, ei.mouse.y);
						essence_->widget_to_header(pos);
						drawer_header_->grab(pos, false);
						draw();
						API::lazy_refresh();
						API::capture_window(essence_->lister.wd_ptr()->handle(), false);
					}
				}

				void trigger::mouse_wheel(graph_reference graph, const eventinfo& ei)
				{
					if(essence_->wheel(ei.wheel.upwards))
					{
						draw();
						essence_->adjust_scroll_value();
						API::lazy_refresh();
					}
				}

				void trigger::dbl_click(graph_reference graph, const eventinfo& ei)
				{
					if(essence_->pointer_where.first == essence_t::where::lister)
					{
						std::pair<es_lister::size_type, es_lister::size_type> item;
						//Get the item which the mouse is placed.
						if(essence_->lister.forward(essence_->scroll.offset_y.x, essence_->scroll.offset_y.y, essence_->pointer_where.second, item))
						{
							if(item.second == npos)	//being the npos of item.second is a category
							{
								bool do_expand = (essence_->lister.expand(item.first) == false);
								essence_->lister.expand(item.first, do_expand);

								if(do_expand == false)
								{
									std::pair<size_type, size_type> last = essence_->lister.last();
									size_type n = essence_->number_of_lister_items(false);
									if(essence_->lister.backward(last.first, last.second, n, last))
									{
										essence_->scroll.offset_y.x = static_cast<nana::upoint::value_type>(last.first);
										essence_->scroll.offset_y.y = static_cast<nana::upoint::value_type>(last.second);
									}
								}
								essence_->adjust_scroll_life();
								draw();
								API::lazy_refresh();
							}
						}
					}
				}
				void trigger::resize(graph_reference graph, const eventinfo& ei)
				{
					essence_->adjust_scroll_life();
					draw();
					API::lazy_refresh();
				}

				void trigger::key_down(graph_reference graph, const eventinfo& ei)
				{
					switch(ei.keyboard.key)
					{
					case keyboard::os_arrow_up:
					case keyboard::os_arrow_down:
						essence_->lister.move_select(ei.keyboard.key == keyboard::os_arrow_up);
						essence_->trace_selected_item();
						draw();
						API::lazy_refresh();
						break;
					}
				}
			//end class trigger
		}
	}//end namespace drawerbase

	//class listbox
		listbox::listbox(){}

		listbox::listbox(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		listbox::listbox(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		listbox::ext_event_type& listbox::ext_event() const
		{
			return get_drawer_trigger().essence().lister.ext_event;
		}

		void listbox::auto_draw(bool ad)
		{
			get_drawer_trigger().essence().set_auto_draw(ad);
		}

		void listbox::append_categ(const nana::string& text)
		{
			get_drawer_trigger().essence().lister.create(text);
			get_drawer_trigger().update();
		}

		void listbox::append_header(const nana::string& text, unsigned width)
		{
			get_drawer_trigger().essence().header.create(text, width);
			get_drawer_trigger().update();
		}

		void listbox::append_item(const nana::string& text)
		{
			append_item(0, text);
		}

		void listbox::append_item(listbox::size_type cat, const nana::string& text)
		{
			drawerbase::listbox::es_lister & lister = get_drawer_trigger().essence().lister;
			lister.push_back(cat, text);
			window wd = handle();
			if(false == API::empty_window(wd))
			{
				drawerbase::listbox::es_lister::item_t & item = lister.at(cat, lister.size_item(cat) - 1);
				item.bkcolor = API::background(wd);
				item.fgcolor = API::foreground(wd);
				get_drawer_trigger().update();
			}
		}

		void listbox::insert(size_type cat, size_type index, const nana::string& text)
		{
			drawerbase::listbox::es_lister & lister = get_drawer_trigger().essence().lister;
			if(lister.insert(cat, index, text))
			{
				window wd = handle();
				if(false == API::empty_window(wd))
				{
					drawerbase::listbox::es_lister::item_t& item = lister.at(cat, index);
					item.bkcolor = API::background(wd);
					item.fgcolor = API::foreground(wd);
					get_drawer_trigger().update();
				}
			}
		}

		void listbox::checkable(bool chkable)
		{
			if(get_drawer_trigger().essence().checkable != chkable)
			{
				get_drawer_trigger().essence().checkable = chkable;
				get_drawer_trigger().update();
			}
		}

		bool listbox::checked(listbox::size_type item) const
		{
			return get_drawer_trigger().essence().lister.at(0, item).flags.checked;
		}

		bool listbox::checked(listbox::size_type cat, listbox::size_type item) const
		{
			return get_drawer_trigger().essence().lister.at(cat, item).flags.checked;
		}

		void listbox::checked(std::vector<std::pair<listbox::size_type, listbox::size_type> >& vec)
		{
			get_drawer_trigger().essence().lister.item_checked(vec);
		}

		void listbox::checked(size_type cat, size_type i, bool value)
		{
			drawerbase::listbox::es_lister & lister = get_drawer_trigger().essence().lister;
			drawerbase::listbox::es_lister::item_t & item = lister.at_abs(cat, i);
			if(item.flags.checked != value)
			{
				item.flags.checked = value;
				lister.ext_event.checked(*lister.wd_ptr(), cat, i, value);
				get_drawer_trigger().update();
			}
		}

		void listbox::clear(size_type categ)
		{
			drawerbase::listbox::essence_t & es = get_drawer_trigger().essence();
			es.lister.clear(categ);
			nana::upoint pos = es.scroll_y();
			if(pos.x == categ)
			{
				pos.y = (pos.x > 0 ? drawerbase::listbox::essence_t::where::unknown : 0);
				es.scroll_y(pos);
			}
			get_drawer_trigger().update();
		}

		void listbox::clear()
		{
			drawerbase::listbox::essence_t & es = get_drawer_trigger().essence();
			es.lister.clear();
			nana::upoint pos = es.scroll_y();
			pos.y = (pos.x > 0 ? drawerbase::listbox::essence_t::where::unknown : 0);
			es.scroll_y(pos);
			get_drawer_trigger().update();
		}

		void listbox::erase(size_type categ, size_type item)
		{
			drawerbase::listbox::essence_t & es = get_drawer_trigger().essence();
			es.lister.erase(categ, item);
			nana::upoint pos = es.scroll_y();
			if((pos.x == categ) && (item <= pos.y))
			{
				if(pos.y == 0)
				{
					if(es.lister.size_item(categ) == 0)
						pos.y = (pos.x > 0 ? drawerbase::listbox::essence_t::where::unknown : 0);
				}
				else
					--pos.y;
				es.scroll_y(pos);
			}
			get_drawer_trigger().update();
		}

		void listbox::erase(size_type categ)
		{
			drawerbase::listbox::essence_t & es = get_drawer_trigger().essence();
			es.lister.erase(categ);
			if(categ)
			{
				nana::upoint pos = es.scroll_y();
				if(categ <= pos.x)
				{
					if(pos.x == es.lister.size_categ())
						--pos.x;
					pos.y = drawerbase::listbox::essence_t::where::unknown;
					es.scroll_y(pos);
				}
			}
			else
				es.scroll_y(nana::upoint(0, 0));
			get_drawer_trigger().update();
		}

		void listbox::erase()
		{
			drawerbase::listbox::essence_t & es = get_drawer_trigger().essence();
			es.lister.erase();
			es.scroll_y(nana::upoint(0, 0));
			get_drawer_trigger().update();
		}

		nana::string listbox::item_text(size_type categ, size_type index, size_type sub) const
		{
			return get_drawer_trigger().essence().lister.item_text(categ, index, sub);
		}

		void listbox::set_item_text(listbox::size_type index, listbox::size_type sub, const nana::string& text)
		{
			set_item_text(0, index, sub, text);
		}

		void listbox::set_item_text(listbox::size_type categ, listbox::size_type index, listbox::size_type sub, const nana::string& text)
		{
			drawerbase::listbox::essence_t & es = get_drawer_trigger().essence();
			es.lister.text(categ, index, sub, text, es.header.cont().size());
			get_drawer_trigger().update();
		}

		void listbox::set_sort_compare(size_type sub, const nana::functor<bool(const nana::string&, nana::any*, const nana::string&, nana::any*, bool reverse)>& strick_ordering)
		{
			get_drawer_trigger().essence().header.get_item(sub).weak_ordering = strick_ordering;
		}

		void listbox::show_header(bool sh)
		{
			get_drawer_trigger().essence().header.visible(sh);
			get_drawer_trigger().update();
		}

		bool listbox::visible_header() const
		{
			return get_drawer_trigger().essence().header.visible();
		}

		bool listbox::selected(listbox::size_type item) const
		{
			return get_drawer_trigger().essence().lister.at(0, item).flags.selected;
		}

		bool listbox::selected(listbox::size_type cat, listbox::size_type i) const
		{
			return get_drawer_trigger().essence().lister.at(cat, i).flags.selected;
		}

		void listbox::selected(std::vector<std::pair<listbox::size_type, listbox::size_type> >& vec)
		{
			get_drawer_trigger().essence().lister.item_selected(vec);
		}

		void listbox::selected(size_type cat, size_type i, bool value)
		{
			drawerbase::listbox::es_lister & lister = get_drawer_trigger().essence().lister;
			drawerbase::listbox::es_lister::item_t & item = lister.at_abs(cat, i);
			if(item.flags.selected != value)
			{
				item.flags.selected = value;
				lister.ext_event.selected(*lister.wd_ptr(), cat, i, value);
				get_drawer_trigger().update();
			}
		}

		void listbox::move_select(bool upwards)
		{
			get_drawer_trigger().essence().lister.move_select(upwards);
			get_drawer_trigger().update();
		}

		void listbox::icon(size_type cat, size_type index, const nana::paint::image& img)
		{
			if(img)
			{
				get_drawer_trigger().essence().lister.at(cat, index).img = img;
				get_drawer_trigger().essence().if_image = true;
				get_drawer_trigger().update();
			}
		}

		nana::paint::image listbox::icon(size_type cat, size_type index) const
		{
			return get_drawer_trigger().essence().lister.at(cat, index).img;
		}

		void listbox::item_background(size_type cat, size_type index, nana::color_t color)
		{
			get_drawer_trigger().essence().lister.at(cat, index).bkcolor = color;
			get_drawer_trigger().update();
		}

		nana::color_t listbox::item_background(size_type cat, size_type index) const
		{
			return get_drawer_trigger().essence().lister.at(cat, index).bkcolor;
		}

		void listbox::item_foreground(size_type cat, size_type index, nana::color_t color)
		{
			get_drawer_trigger().essence().lister.at(cat, index).fgcolor = color;
			get_drawer_trigger().update();
		}

		nana::color_t listbox::item_foreground(size_type cat, size_type index) const
		{
			return get_drawer_trigger().essence().lister.at(cat, index).fgcolor;
		}

		listbox::size_type listbox::size_categ() const
		{
			return get_drawer_trigger().essence().lister.size_categ();
		}

		listbox::size_type listbox::size_item() const
		{
			return size_item(0);
		}

		listbox::size_type listbox::size_item(listbox::size_type categ) const
		{
			return get_drawer_trigger().essence().lister.size_item(categ);
		}

		nana::any* listbox::_m_anyobj(size_type cat, size_type index, bool allocate_if_empty) const
		{
			return get_drawer_trigger().essence().lister.anyobj(cat, index, allocate_if_empty);
		}

		void listbox::_m_resolver(const nana::any& res)
		{
			get_drawer_trigger().essence().resolver = res;
		}

		const nana::any & listbox::_m_resolver() const
		{
			return get_drawer_trigger().essence().resolver;
		}

		std::size_t listbox::_m_headers() const
		{
			return get_drawer_trigger().essence().header.cont().size();
		}
	//end class listbox
}//end namespace gui
}//end namespace nana
