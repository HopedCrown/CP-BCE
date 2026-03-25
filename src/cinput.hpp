#include <alias.h>
#include <gint/display.h>
#include <vector>

// Colors
public color C_WHITE = 0xffff;
public color C_DARK = 0x528a;

// SCREEN Dimensions and other base Constants

#define SCREEN_H 528
#define SCREEN_W 320

#define NUM_LIST ["1","2","3","4","5","6","7","8","9","0"] 

#define KBD_H 260
#define TAB_H 30
#define PICK_HEADER_H 40
#define PICK_FOOTER_H 45
#define PICK_ITEM_H 50

// THEMES

/*
Safe method for conversion to rgb to either rgb 565 or rgb 555
*/
public inline int[] safe_rgb(int r,int  g,int  b){return C_RGB(r,g,b)};

// THEME  type for custom themes
public typedef struct THEME {
  color modal_bg;
  color kbd_bg;
  color key_bg;
  color key_spec;
  color key_out;
  color txt;
  color txt_dim;
  color accent;
  color txt_acc;
  color hl;
  color check;
} THEME;

/*
struct of all supported themes (built-in)
*/
public typedef struct themes {
  THEME light; 
  THEME dark;
  THEME grey;
} themes;
/*

THEME light = {
  C_WHITE, 
  C_WHITE,
  safe_rgb(28,29,28),
  C_DARK,
  safe_rgb(4, 4, 4),
  safe_rgb(8, 8, 8),
  safe_rgb(1, 11, 26),
  C_WHITE,
  safe_rgb(28, 29, 28),
  C_WHITE
}; // I think this is how you make an  instance of a struct ? -> No it is not, but VERY close.

*/
struct rect_t {
    int x;
    int y;
    int w;
    int h;
};

// Item (matches Python dict)
struct L_ITEM {
    const char* text;
    const char* type; // "item" or "section"
    int height;
    int computed_height;
    int computed_y;
};

class ListView {
public:
  ListView();
private:
  rect_t rect;
  L_ITEM items;
  int row_h = 40;
  THEME theme = themes.light;
  int headers_h = 0;
}

/*

class ListView:
    def __init__(self, rect, items, row_h=40, theme='light', headers_h=None):
        """
        rect: (x, y, w, h) tuple
        items: List of dicts {'text': str, 'type': 'item'|'section', 'height': int, ...} OR list of strings
        """
        self.x, self.y, self.w, self.h = rect
        # Normalize items
        self.items = []
        for it in items:
            if isinstance(it, dict): self.items.append(it)
            else: self.items.append({'text': str(it), 'type': 'item'})
            
        self.base_row_h = row_h
        self.headers_h = headers_h if headers_h else row_h
        self.theme = get_theme(theme)
        
        # Layout State
        self.total_h = 0
        self.recalc_layout()
        
        # Selection & Scroll
        self.selected_index = -1
        self.scroll_y = 0 
        self.max_scroll = max(0, self.total_h - self.h)
        
        # Select first selectable item
        self.select_next(0, 1)

        # Touch State
        self.is_dragging = False
        self.touch_start_y = 0
        self.touch_start_idx = 0
        self.touch_start_time = 0
        self.touch_acc_y = 0.0 # Accumulator for drag distance
        self.touch_initial_item_idx = -1
        self.long_press_triggered = False
        
        # Configuration
        self.drag_threshold = 10
        self.long_press_delay = 0.5 # seconds
        self.snap_sensitivity = 1.0 # 1.0 = 1 item height drag moves 1 item

    def recalc_layout(self):
        """Calculate positions and heights of all items."""
        total_h = 0
        for it in self.items:
            h = it.get('height', self.headers_h if it.get('type') == 'section' else self.base_row_h)
            it['_h'] = h
            it['_y'] = total_h
            total_h += h
        self.total_h = total_h
        self.max_scroll = max(0, self.total_h - self.h)

    def select_next(self, start_idx, step):
        """Find next selectable item"""
        idx = start_idx
        count = len(self.items)
        if count == 0: 
            self.selected_index = -1
            return
            
        # Safety loop limit
        for _ in range(count):
            if 0 <= idx < count:
                if self.items[idx].get('type', 'item') != 'section':
                    self.selected_index = idx
                    self.ensure_visible()
                    return
            idx += step
            # Clamp logic
            if idx < 0 or idx >= count: break

    def ensure_visible(self):
        """Scroll to keep selected_index in view"""
        if self.selected_index < 0 or self.selected_index >= len(self.items): return
        
        it = self.items[self.selected_index]
        item_top = it['_y']
        item_bot = item_top + it['_h']
        
        view_top = self.scroll_y
        view_bot = self.scroll_y + self.h
        
        if item_top < view_top:
            self.scroll_y = item_top
        elif item_bot > view_bot:
            self.scroll_y = item_bot - self.h
            
        self.clamp_scroll()

    def clamp_scroll(self):
        self.max_scroll = max(0, self.total_h - self.h)
        self.scroll_y = max(0, min(self.max_scroll, self.scroll_y))

    def update(self, events):
        """
        Process events.
        """
        now = time.monotonic()
        
        # Touch Handling
        touch_up = None
        touch_down = None
        current_touch = None
        
        # Pre-process touch events
        for e in events:
            if e.type == KEYEV_TOUCH_DOWN:
                touch_down = e
                current_touch = e
            elif e.type == KEYEV_TOUCH_UP:
                touch_up = e
            elif e.type == KEYEV_TOUCH_DRAG: # Some envs might emit this
                current_touch = e

        # Also look for simulated drag via repeatedly polled DOWN events if native drag not available
        if not current_touch and touch_down:
            current_touch = touch_down
            
        # 1. Start Touch
        if touch_down and not self.is_dragging and self.touch_start_time == 0:
            if self.x <= touch_down.x < self.x + self.w and self.y <= touch_down.y < self.y + self.h:
                self.touch_start_y = touch_down.y
                self.touch_start_time = now
                
                # Determine which item was touched initially
                local_y = touch_down.y - self.y + self.scroll_y
                self.touch_initial_item_idx = self.get_index_at(local_y)
                
                # Anchor drag to the item under finger if valid, else selection
                if self.touch_initial_item_idx != -1:
                    self.touch_start_idx = self.touch_initial_item_idx
                    # Immediate visual feedback
                    if 0 <= self.touch_initial_item_idx < len(self.items):
                        if self.items[self.touch_initial_item_idx].get('type') != 'section':
                            self.selected_index = self.touch_initial_item_idx
                            self.ensure_visible()
                else:
                    self.touch_start_idx = self.selected_index
                
                self.is_dragging = False
                self.long_press_triggered = False
                self.touch_acc_y = 0.0

        # Long Press Detection (Time-based)
        if self.touch_start_time != 0 and not self.is_dragging and not self.long_press_triggered:
            if now - self.touch_start_time > 0.8: # 800ms threshold
                self.long_press_triggered = True
                # Return 'long' action immediately
                idx = self.selected_index
                if 0 <= idx < len(self.items):
                    return ('long', idx, self.items[idx])

        # ... Or use the last known touch if we are already in a touch sequence
        # We need a way to know "current finger position" even if no new event came this frame, 
        # but typically we get continuous events. If we don't, we can't drag.
        
        # 2. Touch Move / Drag
        if self.touch_start_time != 0:
             # Find the most recent position event
             last_pos = current_touch if current_touch else touch_down
             
             if last_pos:
                dy = last_pos.y - self.touch_start_y
                
                # Check for drag threshold
                # User req: "If the drag is more than one item height, then apply scrolling"
                if not self.is_dragging:
                    if abs(dy) > self.base_row_h: # Threshold is one item height
                        self.is_dragging = True
                        self.long_press_triggered = True
                
                if self.is_dragging:
                    # Scroll Logic (Snap)
                    # Dragging UP (negative dy) -> Move down in list
                    
                    # More advanced "Pixel-based" mapping to index
                    # We map the total pixel offset (start_y + scroll_y_offset - dy) to an index
                    # This allows variable row heights natural feeling
                    
                    # 1. Calculate theoretical pixel position
                    # We want to move the "originally selected item" by -dy pixels relative to the view center?
                    # Or simpler: The "selection cursor" moves by -dy pixels.
                    
                    # Current selection top in pixels
                    if 0 <= self.touch_start_idx < len(self.items):
                        start_item_y = self.items[self.touch_start_idx]['_y']
                        
                        # Target Y for the selection top
                        target_y = start_item_y - dy
                        
                        # Find index at target_y
                        # Wescan items to find which one contains target_y
                        # But to fix the "clamping" issue with sections:
                        # If the index found is a section, or we are crossing a section...
                        
                        found_idx = -1
                        for i, it in enumerate(self.items):
                            if it['_y'] <= target_y < it['_y'] + it['_h']:
                                found_idx = i
                                break
                        
                        if found_idx == -1:
                           if target_y < 0: found_idx = 0
                           else: found_idx = len(self.items) - 1
                        
                        # Apply Clamping Logic:
                        # If found_idx is a SECTION, we check where in the section we are.
                        # User wants: "consider the selection to enter the next section first item 
                        # only when the scroll amount is above the title height."
                        
                        # Implementation interpretation: 
                        # If we land on a section header (found_idx is Section),
                        # we should STAY on the *previous* item unless we are past the section header?
                        # Or STAY on the section header (But sections aren't selectable...)?
                        # "Selectable" logic usually skips sections.
                        
                        # Refined logic: Drag maps to a pixel Y. That pixel Y lands on an item.
                        # If it lands on a Section:
                        #   If we came from above (moving down), we need to drag PAST the section entirely to select the item below it.
                        #   If we came from below (moving up), we need to drag ABOVE the section entirely to select item above.
                        
                        # Effectively, the "active area" for the section header does not trigger selection change until passed?
                        # Let's try: if match is section, map to previous valid item if (target_y < section_center) else next valid item?
                        # User said: "only when the scroll amount is above the title height"
                        
                        if self.items[found_idx].get('type') == 'section':
                             # We are hovering over a section
                             # Check overlap
                             sec_y = self.items[found_idx]['_y']
                             sec_h = self.items[found_idx]['_h']
                             overlap = target_y - sec_y
                             
                             # If we are dragging DOWN (dy < 0, target_y increasing), we are moving to next section
                             if dy < 0: 
                                 # We are moving down the list.
                                 # Only jump to next item if we are really deep into the section or past it?
                                 # Actually, if we are conceptually "dragging the paper", moving finger UP (negative dy) 
                                 # means we want to see lower items.
                                 pass
                             
                             # Logic: 
                             # If we land on section, look at adjacent items.
                             # If we are closer to the top of section, pick item above.
                             # If we are closer to bottom, pick item below.
                             # "Glitch: separator ... flicker between the two voices"
                             
                             # Let's enforce a "dead zone" corresponding to the section height.
                             # If target_y falls within a section's vertical space, keep the selection 
                             # on the *previously selected item* (from start of drag or previous frame)
                             # UNLESS we have crossed it?
                             
                             # Simpler: Don't select the section. Select the adjacent valid item based on direction, 
                             # BUT require the "target_y" to be fully into the valid item's space.
                             
                             # If target_y is in section -> Don't change selection from what it would be if we were at the edge?
                             
                             if self.selected_index < found_idx:
                                  # We were above, moving down.
                                  # Stay above until we are purely past the section?
                                  # effectively, for the section to be skipped, target_y must be >= sec_y + sec_h
                                  # But found_idx says we are < sec_y + sec_h.
                                  # So stay at found_idx - 1 (or whatever was valid above)
                                  final_idx = found_idx - 1
                             else:
                                  # We were below, moving up.
                                  # Stay below until target_y < sec_y
                                  final_idx = found_idx + 1
                             
                             # Boundary checks
                             final_idx = max(0, min(len(self.items)-1, final_idx))
                        
                        else:
                             final_idx = found_idx

                        if 0 <= final_idx < len(self.items) and self.items[final_idx].get('type') != 'section':
                            self.selected_index = final_idx
                            self.ensure_visible()

        # 3. Touch Release
        if touch_up:
            if self.touch_start_time != 0:
                # Valid release sequence
                ret = None
                
                if not self.is_dragging and not self.long_press_triggered:
                    # Click Candidate
                    local_y = touch_up.y - self.y + self.scroll_y
                    release_idx = self.get_index_at(local_y)
                    
                    # Logic: If release is on same item as start, it's a click.
                    if release_idx == self.touch_initial_item_idx and release_idx >= 0:
                         if self.items[release_idx].get('type') != 'section':
                             # Ensure selection updates to the clicked item
                             self.selected_index = release_idx
                             self.ensure_visible()
                             ret = ('click', release_idx, self.items[release_idx])
                
                # Reset
                self.touch_start_time = 0
                self.is_dragging = False
                return ret
                
            # If touch_up happened but we weren't tracking, ignore it (stray event)

        # 4. Long Press
        if self.touch_start_time != 0 and not self.is_dragging and not self.long_press_triggered:
            if now - self.touch_start_time > self.long_press_delay:
                self.long_press_triggered = True
                # Trigger on current selection or initial?
                # Usually initial item
                if 0 <= self.touch_initial_item_idx < len(self.items):
                     it = self.items[self.touch_initial_item_idx]
                     if it.get('type') != 'section':
                         self.selected_index = self.touch_initial_item_idx
                         return ('long', self.touch_initial_item_idx, it)

        # 5. Keys
        for e in events:
            if e.type == KEYEV_DOWN or (e.type == KEYEV_HOLD and e.key in [KEY_UP, KEY_DOWN]): 
                if e.key == KEY_UP:
                    self.select_next(self.selected_index - 1, -1)
                elif e.key == KEY_DOWN:
                    self.select_next(self.selected_index + 1, 1)
                elif e.key == KEY_EXE:
                    if self.selected_index >= 0:
                        return ('click', self.selected_index, self.items[self.selected_index])
        
        return None

    def get_index_at(self, y):
        # Linear scan is sufficient for likely list sizes (<500)
        # Binary search could be used since _y is sorted
        for i, it in enumerate(self.items):
            if it['_y'] <= y < it['_y'] + it['_h']:
                return i
        return -1

    def draw_item(self, x, y, item, is_selected):
        # Can be overridden by subclass or assigned
        t = self.theme
        h = item['_h']
        
        if item.get('type') == 'section':
            drect(x, y, x + self.w, y + h, t['key_spec'])
            drect_border(x, y, x + self.w, y + h, C_NONE, 1, t['key_spec'])
            dtext_opt(x + 10, y + h//2, t['txt_dim'], C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, str(item['text']), -1)
        else:
            bg = t['hl'] if is_selected else t['modal_bg']
            drect(x, y, x + self.w, y + h, bg)
            drect_border(x, y, x + self.w, y + h, C_NONE, 1, t['key_spec'])
            
            x_off = 20
            if item.get('checked'):
                self.draw_check(x + 10, y + (h-20)//2, t)
                x_off = 40
                
            dtext_opt(x + x_off, y + h//2, t['txt'], C_NONE, DTEXT_LEFT, DTEXT_MIDDLE, str(item['text']), -1)
            
            # Draw Arrow if requested
            if item.get('arrow'):
                 ar_x = x + self.w - 15
                 ar_y = y + h//2
                 c = t['txt_dim']
                 dline(ar_x - 4, ar_y - 4, ar_x, ar_y, c)
                 dline(ar_x - 4, ar_y + 4, ar_x, ar_y, c)

    def draw(self):
        t = self.theme
        drect(self.x, self.y, self.x + self.w, self.y + self.h, t['modal_bg'])
        
        # Lazy Rendering: Find start index
        start_y = self.scroll_y
        end_y = self.scroll_y + self.h
        
        # Simple scan to find start (optimize later if needed)
        start_idx = 0
        for i, it in enumerate(self.items):
            if it['_y'] + it['_h'] > start_y:
                start_idx = i
                break
                
        # Draw visible items
        for i in range(start_idx, len(self.items)):
            it = self.items[i]
            if it['_y'] >= end_y: break # Stop if below view
            
            item_y = self.y + it['_y'] - self.scroll_y
            self.draw_item(self.x, item_y, it, (i == self.selected_index))

        # Scrollbar
        if self.max_scroll > 0:
            sb_w = 4
            ratio = self.h / (self.total_h if self.total_h > 0 else 1)
            thumb_h = max(20, int(self.h * ratio))
            
            scroll_ratio = self.scroll_y / self.max_scroll
            thumb_y = self.y + int(scroll_ratio * (self.h - thumb_h))
            
            sb_x = self.x + self.w - sb_w - 2
            drect(sb_x, thumb_y, sb_x + sb_w, thumb_y + thumb_h, t['accent'])

    def draw_check(self, x, y, t):
        drect(x, y, x+20, y+20, t['accent'])
        c = t['check']
        dline(x+4, y+10, x+8, y+14, c)
        dline(x+8, y+14, x+15, y+5, c)


*/
