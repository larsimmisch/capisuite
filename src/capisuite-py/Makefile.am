pkgsysconfdir = @sysconfdir@/capisuite

python_module_PYTHON = __init__.py config.py consts.py core.py exceptions.py \
    fax.py fileutils.py voice.py

EXTRA_DIST = config.py.in

config.py: config.py.in
	rm -f $@
	sed -e 's,@\pkgsysconfdir@,$(pkgsysconfdir),g' $< >$@

all-local: config.py
