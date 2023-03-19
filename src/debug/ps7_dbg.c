unsigned long ps7_dbg_get_ps_version()
{
	// PS version from MCTRL register bits [31:28]
	unsigned long mask = 0xF0000000;
	unsigned long *addr = NULL;
	unsigned long ps_version = 0;
	addr = (unsigned long *) 0xF8007080;
	mctrl = *addr;
	ps_version = (mctrl & mask) >> 28;
	return ps_version;
}

