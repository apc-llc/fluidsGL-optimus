extern "C"
{
	void on_surface_created();
	void on_surface_changed(int width, int height);
	void on_draw_frame();
	void on_connect(const char* bc_addr);
	void on_reset();
	void on_click(bool clicked, int x, int y);
	void on_motion(int x, int y);
}
