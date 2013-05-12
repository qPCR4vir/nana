/*
 *	An Animation Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/animation.cpp
 */


#include <nana/gui/animation.hpp>
#include <nana/gui/drawing.hpp>
#include <nana/system/timepiece.hpp>

#include <vector>
#include <list>

#include <nana/threads/mutex.hpp>
#include <nana/threads/condition_variable.hpp>
#include <nana/threads/thread.hpp>
#include <stdexcept>

namespace nana{	namespace gui
{
	class animation;

	struct output_t
	{
		drawing::diehard_t diehard;
		std::vector<nana::point> points;

		output_t()
			: diehard(0)
		{}
	};

	struct framebuilder
	{
		std::size_t length;
		nana::functor<bool(std::size_t, paint::graphics&, nana::size&)> frbuilder;

		framebuilder(const nana::functor<bool(std::size_t, paint::graphics&, nana::size&)>& f, std::size_t l)
			: length(1), frbuilder(f)
		{}
	};

	struct frame
	{
		struct kind
		{
			enum t
			{
				oneshot,
				framebuilder
			};
		};

		frame(const paint::image& r)
			: type(kind::oneshot)
		{
			u.oneshot = new paint::image(r);
		}

		frame(const nana::functor<bool(std::size_t, paint::graphics&, nana::size&)>& frbuilder, std::size_t length)
			: type(kind::framebuilder)
		{
			u.frbuilder = new framebuilder(frbuilder, length);
		}

		frame(const frame& r)
			: type(r.type)
		{
			switch(type)
			{
			case kind::oneshot:
				u.oneshot = new paint::image(*r.u.oneshot);
				break;
			case kind::framebuilder:
				u.frbuilder = new framebuilder(*r.u.frbuilder);
				break;
			}
		}

		~frame()
		{
			switch(type)
			{
			case kind::oneshot:
				delete u.oneshot;
				break;
			case kind::framebuilder:
				delete u.frbuilder;
				break;
			}
		}

		frame& operator=(const frame& r)
		{
			if(this != &r)
			{
				switch(type)
				{
				case kind::oneshot:
					delete u.oneshot;
					break;
				case kind::framebuilder:
					delete u.frbuilder;
					break;
				}

				type = r.type;
				switch(type)
				{
				case kind::oneshot:
					u.oneshot = new paint::image(*r.u.oneshot);
					break;
				case kind::framebuilder:
					u.frbuilder = new framebuilder(*r.u.frbuilder);
					break;
				}
			}
			return *this;
		}

		std::size_t length() const
		{
			switch(type)
			{
			case kind::oneshot:
				return 1;
			case kind::framebuilder:
				return u.frbuilder->length;
			}
			return 0;
		}

		//
		kind::t type;
		union uframes
		{
			paint::image * oneshot;
			framebuilder * frbuilder;
		}u;
	};

	//class frameset
		//struct frameset::impl
		struct frameset::impl
		{
			//Only list whos iterator would not invalided after a insertion.
			std::list<frame> frames;
			std::list<frame>::iterator this_frame;
			std::size_t pos_in_this_frame;
			mutable bool good_frame_by_frmbuilder;	//It indicates the state of frame whether is valid.

			impl()
				:	this_frame(frames.end()), pos_in_this_frame(0),
					good_frame_by_frmbuilder(false)
			{}

			class renderer_oneshot
			{
			public:
				renderer_oneshot(frame & frmobj)
					: frmobj_(frmobj)
				{}

				void operator()(paint::graphics& tar, const nana::point& pos)
				{
					frmobj_.u.oneshot->paste(tar, pos.x, pos.y);
				}
			private:
				frame & frmobj_;
			};

			class renderer_frmbuilder
			{
			public:
				renderer_frmbuilder(paint::graphics& framegraph, nana::rectangle& r)
					: framegraph_(framegraph), r_(r)
				{}

				void operator()(paint::graphics& tar, const nana::point& pos)
				{
					r_.x = pos.x;
					r_.y = pos.y;
					tar.bitblt(r_, framegraph_);
				}
			private:
				paint::graphics& framegraph_;
				nana::rectangle & r_;
			};

			//Render A frame on the set of windows.
			void render_this(std::map<window, output_t>& outs, paint::graphics& framegraph, nana::size& framegraph_dimension) const
			{
				if(this_frame == frames.end())
					return;

				frame & frmobj = *this_frame;
				switch(frmobj.type)
				{
				case frame::kind::oneshot:
					_m_render(outs, renderer_oneshot(frmobj));
					break;
				case frame::kind::framebuilder:
					good_frame_by_frmbuilder = frmobj.u.frbuilder->frbuilder(pos_in_this_frame, framegraph, framegraph_dimension);
					if(good_frame_by_frmbuilder)
					{
						nana::rectangle r = framegraph_dimension;
						_m_render(outs, renderer_frmbuilder(framegraph, r));
					}
					break;
				}
			}

			//Render a frame on a specified window graph
			void render_this(paint::graphics& graph, const nana::point& pos, paint::graphics& framegraph, nana::size& framegraph_dimension, bool rebuild_frame) const
			{
				if(this_frame == frames.end())
					return;

				frame & frmobj = *this_frame;
				switch(frmobj.type)
				{
				case frame::kind::oneshot:
					frmobj.u.oneshot->paste(graph, pos.x, pos.y);
					break;
				case frame::kind::framebuilder:
					if(rebuild_frame)
						good_frame_by_frmbuilder = frmobj.u.frbuilder->frbuilder(pos_in_this_frame, framegraph, framegraph_dimension);

					if(good_frame_by_frmbuilder)
					{
						nana::rectangle r(pos, framegraph_dimension);
						graph.bitblt(r, framegraph);
					}
					break;
				}
			}

			bool eof() const
			{
				return (frames.end() == this_frame);
			}

			void next_frame()
			{
				if(frames.end() == this_frame)
					return;

				frame & frmobj = *this_frame;
				switch(frmobj.type)
				{
				case frame::kind::oneshot:
					++this_frame;
					pos_in_this_frame = 0;
					break;
				case frame::kind::framebuilder:
					if(pos_in_this_frame >= frmobj.u.frbuilder->length)
					{
						pos_in_this_frame = 0;
						++this_frame;
					}
					else
						++pos_in_this_frame;
					break;
				default:
					throw std::runtime_error("Nana.GUI.Animation: Bad frame type");
				}
			}

			//Seek to the first frame
			void reset()
			{
				this_frame = frames.begin();
				pos_in_this_frame = 0;
			}
		private:
			template<typename Renderer>
			void _m_render(std::map<window, output_t>& outs, Renderer renderer) const
			{
				for(std::map<window, output_t>::iterator i = outs.begin(); i != outs.end(); ++i)
				{
					paint::graphics * graph = API::dev::window_graphics(i->first);
					if(0 == graph)
						continue;

					for(std::vector<nana::point>::iterator u = i->second.points.begin(), end = i->second.points.end(); u != end; ++u)
						renderer(*graph, *u);

					API::update_window(i->first);
				}
			}
		};//end struct frameset::impl

		void frameset::impl_deleter::operator()(impl* p) const
		{
			delete p;
		}
	//public:
		frameset::frameset()
			: impl_(new impl)
		{}

		void frameset::push_back(const paint::image& m)
		{
			impl * ip = impl_.handle();
			bool located = ip->this_frame != ip->frames.end();
			ip->frames.push_back(m);
			if(false == located)
				ip->this_frame = ip->frames.begin();
		}

		void frameset::push_back(framebuilder&fb, std::size_t length)
		{
			impl * ip = impl_.handle();
			ip->frames.push_back(frame(fb, length));
			if(1 == ip->frames.size())
				ip->this_frame = ip->frames.begin();
		}
	//end class frameset

	//class animation
		class animation::performance_manager
		{
		public:
			typedef nana::threads::lock_guard<nana::threads::recursive_mutex> lock_guard;

			struct thread_variable
			{
				nana::threads::mutex mutex;
				nana::threads::condition_variable condvar;
				std::vector<impl*> animations;

				std::size_t active;				//The number of active animations
				nana::refer<nana::threads::thread*> thread;
				double performance_parameter;
			};

			struct perf_thread
			{
				performance_manager * perf;
				thread_variable * thrvar;

				perf_thread(performance_manager* pm, thread_variable* tv)
					: perf(pm), thrvar(tv)
				{}

				void operator()()
				{
					perf->_m_perf_thread(thrvar);
				}
			};

			thread_variable * insert(impl* p);
			void close(impl* p);
			bool empty() const;
		private:
			void _m_perf_thread(thread_variable* thrvar);
		private:
			mutable nana::threads::recursive_mutex mutex_;
			std::vector<thread_variable*> threads_;
		};	//end class animation::performance_manager

		struct animation::impl
		{
			bool	looped;
			volatile bool	paused;

			std::list<frameset> framesets;
			std::map<std::string, branch_t> branches;
			std::map<window, output_t> outputs;

			paint::graphics framegraph;	//framegraph will be created by framebuilder
			nana::size framegraph_dimension;

			struct state_t
			{
				std::list<frameset>::iterator this_frameset;
			}state;

			performance_manager::thread_variable * thr_variable;
			static performance_manager * perf_manager;

			struct renderer
			{
				impl * self;
				nana::point pos;
				renderer(impl* self, const nana::point& pos)
					: self(self), pos(pos)
				{}

				void operator()(paint::graphics& graph)
				{
					self->render_this_specifically(graph, pos);
				}
			};

			struct clean_when_destroy
			{
				struct impl * imp_ptr;

				clean_when_destroy(animation::impl * p)
					: imp_ptr(p)
				{}

				void operator()(const eventinfo& ei)
				{
					nana::threads::lock_guard<nana::threads::mutex> lock(imp_ptr->thr_variable->mutex);
					imp_ptr->outputs.erase(ei.window);
				}
			};

			impl()
				: looped(false), paused(true)
			{
				state.this_frameset = framesets.begin();

				{
					nana::gui::internal_scope_guard isg;
					if(0 == perf_manager)
						perf_manager = new performance_manager;
				}
				thr_variable = perf_manager->insert(this);
			}

			~impl()
			{
				perf_manager->close(this);
				{
					nana::gui::internal_scope_guard isg;
					if(perf_manager->empty())
					{
						delete perf_manager;
						perf_manager = 0;
					}
				}
			}

			void render_this_specifically(paint::graphics& graph, const nana::point& pos)
			{
				if(state.this_frameset != framesets.end())
					animation::_m_frameset_impl(*state.this_frameset)->render_this(graph, pos, framegraph, framegraph_dimension, false);
			}

			void render_this_frame()
			{
				if(state.this_frameset != framesets.end())
					animation::_m_frameset_impl(*state.this_frameset)->render_this(outputs, framegraph, framegraph_dimension);
			}

			bool move_to_next()
			{
				if(state.this_frameset != framesets.end())
				{
					animation::_m_frameset_impl(*state.this_frameset)->next_frame();
					return (!animation::_m_frameset_impl(*state.this_frameset)->eof());
				}
				return false;
			}

			//Seek to the first frameset
			void reset()
			{
				state.this_frameset = framesets.begin();
				if(state.this_frameset != framesets.end())
					animation::_m_frameset_impl(*state.this_frameset)->reset();
			}
		};//end struct animation::impl

		//class animation::performance_manager
			animation::performance_manager::thread_variable* animation::performance_manager::insert(impl* p)
			{
				lock_guard lock(mutex_);

				for(std::vector<thread_variable*>::iterator i = threads_.begin(); i != threads_.end(); ++i)
				{
					thread_variable *thr = *i;

					nana::threads::lock_guard<nana::threads::mutex> privlock(thr->mutex);

					if(thr->performance_parameter / (thr->animations.size() + 1) <= 43.3)
					{
						thr->animations.push_back(p);
						return thr;
					}
				}

				thread_variable* thr = new thread_variable;
				thr->animations.push_back(p);
				thr->performance_parameter = 0.0;
				nana::threads::thread * thrptr = new nana::threads::thread;
				thr->thread = thrptr;

				thrptr->start(perf_thread(this, thr));

				threads_.push_back(thr);
				return thr;
			}

			void animation::performance_manager::close(impl* p)
			{
				lock_guard lock(mutex_);
				for(std::vector<thread_variable*>::iterator i = threads_.begin(); i != threads_.end(); ++i)
				{
					thread_variable * thr = *i;

					nana::threads::lock_guard<nana::threads::mutex> privlock(thr->mutex);
					std::vector<impl*>::iterator u = std::find(thr->animations.begin(), thr->animations.end(), p);
					if(u != thr->animations.end())
					{
						thr->animations.erase(u);
						return;
					}
				}
			}

			bool animation::performance_manager::empty() const
			{
				lock_guard lock(mutex_);
				for(std::vector<thread_variable*>::const_iterator i = threads_.begin(); i != threads_.end(); ++i)
				{
					if((*i)->animations.size())
						return false;
				}
				return true;
			}

			void animation::performance_manager::_m_perf_thread(thread_variable* thrvar)
			{
				nana::system::timepiece tmpiece;
				while(true)
				{
					thrvar->active = 0;
					tmpiece.start();

					{
						nana::threads::lock_guard<nana::threads::mutex> lock(thrvar->mutex);
						for(std::vector<impl*>::iterator i = thrvar->animations.begin(); i != thrvar->animations.end(); ++i)
						{
							impl * ani = *i;
							if(ani->paused)
								continue;

							ani->render_this_frame();
							if(false == ani->move_to_next())
							{
								if(ani->looped)
								{
									ani->reset();
									++thrvar->active;
								}
							}
							else
								++thrvar->active;
						}
					}

					if(thrvar->active)
					{
						thrvar->performance_parameter = tmpiece.calc();
						if(thrvar->performance_parameter < 43.4)
							nana::system::sleep(static_cast<unsigned>(43.4 - thrvar->performance_parameter));
					}
					else
					{
						//There isn't an active frame, then let the thread
						//wait for a signal for an active animation
						nana::threads::unique_lock<nana::threads::mutex> lock(thrvar->mutex);
						if(0 == thrvar->active)
							thrvar->condvar.wait(lock);
					}
				}
			}
		//end class animation::performance_manager

		animation::animation()
			: impl_(new impl)
		{

		}

		void animation::push_back(const frameset& frms)
		{
			impl_->framesets.push_back(frms);
			if(1 == impl_->framesets.size())
				impl_->state.this_frameset = impl_->framesets.begin();
		}
		/*
		void branch(const std::string& name, const frameset& frms)
		{
			impl_->branches[name].frames = frms;
		}

		void branch(const std::string& name, const frameset& frms, std::function<std::size_t(const std::string&, std::size_t, std::size_t&)> condition)
		{
			auto & br = impl_->branches[name];
			br.frames = frms;
			br.condition = condition;
		}
		*/

		void animation::looped(bool enable)
		{
			if(impl_->looped != enable)
			{
				impl_->looped = enable;
				if(enable)
				{
					nana::threads::unique_lock<nana::threads::mutex> lock(impl_->thr_variable->mutex);
					if(0 == impl_->thr_variable->active)
					{
						impl_->thr_variable->active = 1;
						impl_->thr_variable->condvar.notify_one();
					}
				}
			}
		}

		void animation::play()
		{
			impl_->paused = false;
			nana::threads::unique_lock<nana::threads::mutex> lock(impl_->thr_variable->mutex);
			if(0 == impl_->thr_variable->active)
			{
				impl_->thr_variable->active = 1;
				impl_->thr_variable->condvar.notify_one();
			}
		}

		void animation::pause()
		{
			impl_->paused = true;
		}

		void animation::output(window wd, const nana::point& pos)
		{
			output_t & output = impl_->outputs[wd];

			if(0 == output.diehard)
			{
				drawing dw(wd);
				output.diehard = dw.draw_diehard(impl::renderer(impl_, pos));
				API::make_event<events::destroy>(wd, impl::clean_when_destroy(impl_));
			}
			output.points.push_back(pos);
		}

		frameset::impl* animation::_m_frameset_impl(frameset & p)
		{
			return p.impl_.handle();
		}
	//end class animation


	animation::performance_manager * animation::impl::perf_manager;

}	//end namespace gui
}	//end namespace nana
