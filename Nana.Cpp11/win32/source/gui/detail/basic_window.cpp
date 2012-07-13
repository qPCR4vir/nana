#include <nana/gui/detail/basic_window.hpp>
#include <nana/gui/detail/native_window_interface.hpp>

namespace nana{	namespace gui{
	namespace detail
	{
		//class caret_descriptor
			caret_descriptor::caret_descriptor(core_window_t* wd, unsigned width, unsigned height)
				:wd_(wd), size_(width, height), visible_(false), real_visible_state_(false), out_of_range_(false)
			{}

			caret_descriptor::~caret_descriptor()
			{
				if(wd_)	native_interface::caret_destroy(wd_->root);
			}

			void caret_descriptor::set_active(bool active)
			{
				if(wd_)
				{
					if(active)
					{
						native_interface::caret_create(wd_->root, size_.width, size_.height);
						real_visible_state_ = false;
						this->position(point_.x, point_.y);
					}
					else
						native_interface::caret_destroy(wd_->root);

					wd_->root_widget->other.attribute.root->ime_enabled = active;
				}
			}

			auto caret_descriptor::window() const ->core_window_t*
			{
				return wd_;
			}

			void caret_descriptor::position(int x, int y)
			{
				point_.x = x;
				point_.y = y;

				_m_real_paint();
			}

			void caret_descriptor::effective_range(nana::rectangle rect)
			{
				//Chech rect
				if(	(rect.width && rect.height) &&
					(rect.x + rect.width > 0) &&
					(rect.y + rect.height > 0))
				{
					if(rect.x < 0)
					{
						rect.width += rect.x;
						rect.x = 0;
					}

					if(rect.y < 0)
					{
						rect.height += rect.y;
						rect.y = 0;
					}

					if(effective_range_ != rect)
					{
						effective_range_ = rect;
						_m_real_paint();
					}
				}
			}

			nana::point caret_descriptor::position() const
			{
				return point_;
			}

			void caret_descriptor::visible(bool isshow)
			{
				if(visible_ != isshow)
				{
					visible_ = isshow;
					if(visible_ == false || false == out_of_range_)
						_m_visible(isshow);
				}
			}

			bool caret_descriptor::visible() const
			{
				return visible_;
			}

			nana::size caret_descriptor::size() const
			{
				return size_;
			}

			void caret_descriptor::size(const nana::size& s)
			{
				size_ = s;
				_m_real_paint();

				if(visible_)	this->visible(true);
			}

			void caret_descriptor::_m_visible(bool isshow)
			{
				if(real_visible_state_ != isshow)
				{
					real_visible_state_ = isshow;
					native_interface::caret_visible(wd_->root, isshow);
				}
			}

			void caret_descriptor::_m_real_paint()
			{
				nana::point pos = point_;
				nana::size	size = size_;

				nana::rectangle rect = effective_range_;
				if(0 == effective_range_.width || 0 == effective_range_.height)
				{
					rect.x = rect.y = 0;
					rect.width = wd_->rect.width;
					rect.height = wd_->rect.height;
				}
				else
				{
					pos.x += effective_range_.x;
					pos.y += effective_range_.y;
				}

				if(	(pos.x + static_cast<int>(size.width) <= rect.x) || (pos.x >= rect.x + static_cast<int>(rect.width)) ||
					(pos.y + static_cast<int>(size.height) <= rect.y) || (pos.y >= rect.y + static_cast<int>(rect.height))
					)
				{//Out of Range without overlap
					if(false == out_of_range_)
					{
						out_of_range_ = true;

						if(visible_)
							_m_visible(false);
					}
				}
				else
				{
					if(pos.x < rect.x)
					{
						size.width -= (rect.x - pos.x);
						pos.x = rect.x;
					}
					else if(pos.x + size.width > rect.x + rect.width)
					{
						size.width -= pos.x + size.width - (rect.x + rect.width);
					}

					if(pos.y < rect.y)
					{
						size.width -= (rect.y - pos.y);
						pos.y = rect.y;
					}
					else if(pos.y + size.height > rect.y + rect.height)
						size.height -= pos.y + size.height - (rect.y + rect.height);

					if(out_of_range_)
					{
						if(paint_size_ == size)
							_m_visible(true);

						out_of_range_ = false;
					}

					if(paint_size_ != size)
					{
						native_interface::caret_destroy(wd_->root);
						native_interface::caret_create(wd_->root, size.width, size.height);
						real_visible_state_ = false;
						if(visible_)
							_m_visible(true);

						paint_size_ = size;
					}
				
					native_interface::caret_pos(wd_->root, wd_->root_x + pos.x, wd_->root_y + pos.y);
				}
			}
		//end class caret_descriptor

		//struct basic_window
			//struct basic_window::other_tag
				basic_window::other_tag::other_tag(gui::category::flags categ)
					: category(categ), active_window(nullptr), upd_state(update_state::none)
				{
					switch(categ)
					{
					case nana::gui::category::root_tag::value:
						attribute.root = new attr_root_tag;
						attribute.root->focus	= 0;
						attribute.root->menubar	= 0;
						attribute.root->context.focus_changed = false;
						attribute.root->ime_enabled = false;
						break;
					case nana::gui::category::frame_tag::value:
						attribute.frame = new attr_frame_tag;
						attribute.frame->container = 0;
						break;
					default:
						attribute.root = 0;
					}
				}

				basic_window::other_tag::~other_tag()
				{
					switch(category)
					{
					case nana::gui::category::root_tag::value:
						delete attribute.root;
						break;
					case nana::gui::category::frame_tag::value:
						delete attribute.frame;
						break;
					}
				}
			//end struct basic_window::other_tag

			//basic_window
			//@brief: constructor for the root window
			basic_window::basic_window(basic_window* owner, gui::category::root_tag**)
				: other(category::root_tag::value)
			{
				_m_init_pos_and_size(0, rectangle());
				//wait for constexpr
				this->other.category = category::root_tag::value;
				this->_m_initialize(owner);
			}

			basic_window::~basic_window()
			{
				delete together.caret;
				together.caret = 0;
			}

			//bind_native_window
			//@brief: bind a native window and baisc_window
			void basic_window::bind_native_window(native_window_type wd, unsigned width, unsigned height, unsigned extra_width, unsigned extra_height, nana::paint::graphics& graphics)
			{
				if(category::root_tag::value == this->other.category)
				{
					this->root = wd;
					this->rect.width = width;
					this->rect.height = height;
					this->extra_width = extra_width;
					this->extra_height = extra_height;
					this->root_widget = this;
					this->root_graph = &graphics;
				}
			}

			void basic_window::frame_window(native_window_type wd)
			{
				if(category::frame_tag::value == this->other.category)
					other.attribute.frame->container = wd;
			}

			void basic_window::_m_init_pos_and_size(basic_window* parent, const rectangle& r)
			{
				this->rect.width = r.width;
				this->rect.height = r.height;
				this->rect.x = this->root_x = r.x;
				this->rect.y = this->root_y = r.y;

				if(parent)
				{
					this->root_x += parent->root_x;
					this->root_y += parent->root_y;
				}
			}

			void basic_window::_m_initialize(basic_window* parent)
			{
				if(this->other.category == category::root_tag::value)
				{
					if(parent && (nana::system::this_thread_id() != parent->thread_id))
						parent = 0;

					while(parent && (parent->other.category != nana::gui::category::root_tag::value))
						parent = parent->parent;
				
					this->owner = parent;
					this->parent = 0;
					this->index = 0;
				}
				else
				{
					this->parent = parent;
					this->owner = 0;
					this->root_widget = parent->root_widget;
					this->root = parent->root;
					this->root_graph = parent->root_graph;
					this->index = static_cast<unsigned>(parent->children.size());
					parent->children.push_back(this);
				}

				this->flags.capture = false;
				this->flags.dbl_click = true;
				this->flags.enabled = true;
				this->flags.modal = false;
				this->flags.glass = false;
				this->flags.take_active = true;
				this->flags.dropable = false;
				this->flags.fullscreen = false;
				this->flags.tab = nana::gui::detail::tab_type::none;
				this->flags.action = mouse_action::normal;
			
				this->visible = false;

				this->color.foreground = 0x0;
				this->color.background = nana::gui::color::button_face;
				this->color.active = 0x60C8FD;

				this->effect.edge_nimbus = effects::edge_nimbus_none;

				this->together.caret = 0;
				this->flags.refreshing = false;
				this->flags.destroying = false;

				this->extra_width = 0;
				this->extra_height = 0;

				//The window must keep its thread_id same as its parent if it is a child.
				//Otherwise, its root buffer would be mapped repeatly if it is in its parent thread.
				this->thread_id = nana::system::this_thread_id();
				if(parent && (this->thread_id != parent->thread_id))
					this->thread_id = parent->thread_id;
			}
		//end struct basic_window
	}//end namespace detail
}//end namespace gui
}//end namespace nana
