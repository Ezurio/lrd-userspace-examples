/*
 * (C) Copyright 2019
 *
 * SPDX-License-Identifier:     GPL-2.0-or-later
 */

/*
 * The library wraps swupdate client APIs
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "progress_ipc.h"
#include "network_ipc.h"

#include <Python.h>

static int fd = -1;
static int dryrun = 1;
static int msg_fd = -1;

static PyObject* prepare_fw_update(PyObject * self, PyObject * args)
{
	int flag;

	if (!PyArg_ParseTuple(args, "i", &dryrun)) {
		return NULL;
	}

	if(fd > 0)
		ipc_end(fd);

	fd = ipc_inst_start_ext(SOURCE_UNKNOWN, 0, NULL, dryrun);

	if(msg_fd > 0)
		close(msg_fd);

	msg_fd = progress_ipc_connect(false);
	flag = fcntl(msg_fd, F_GETFL, 0);
	flag |= O_NONBLOCK;
	fcntl(msg_fd, F_SETFL, flag);

    return Py_BuildValue("i", fd);
}

static PyObject* do_fw_update(PyObject * self, PyObject * args){

	Py_ssize_t size = 0;
	const char *buf = NULL;
	int rc = -1;

	if (!PyArg_ParseTuple(args, "s#", &buf, &size)){
		return NULL;
	}

	rc = ipc_send_data(fd, (char *)buf, size);
    return Py_BuildValue("i", rc);
}

static PyObject* end_fw_update(PyObject * self, PyObject *Py_UNUSED(ignored)){

	ipc_end(fd);
	Py_RETURN_NONE;
}

static PyObject* get_fw_update_state(PyObject * self, PyObject *Py_UNUSED(ignored))
{
	int rc = 0, valid = 0;
	struct progress_msg msg;
	fd_set rd_set;
	struct timeval timeout;
	int max_fd = msg_fd;

	if(msg_fd < 0) Py_RETURN_NONE;

	while(1){

		FD_ZERO (&rd_set);
		FD_SET (msg_fd, &rd_set);

		/* Initialize the timeout data structure. */
		timeout.tv_sec = 0;
		timeout.tv_usec = 200000;

		rc = select (max_fd + 1, &rd_set, NULL, NULL, &timeout);
		if(rc > 0 && FD_ISSET(msg_fd, &rd_set)){
			read(msg_fd, &msg, sizeof(msg));
			valid = 1;
			continue;
		}

		break;
	}

	if(!valid) Py_RETURN_NONE;
	/* Build the output tuple */
    return Py_BuildValue("iIIIs", msg.status,
			msg.nsteps, msg.cur_step, msg.cur_percent,
			msg.cur_image);
}

static PyMethodDef swclient_methods[] = {
    {"prepare_fw_update", prepare_fw_update, METH_VARARGS, "Prepare to update firmware"},
    {"do_fw_update", do_fw_update, METH_VARARGS, "Do firmware update"},
    {"end_fw_update", end_fw_update, METH_NOARGS, "End firmware update"},
    {"get_fw_update_state", get_fw_update_state, METH_NOARGS, "Get firmware update progress state"},
    {NULL, NULL, 0, NULL}
};

// Module definition
#if PY_MAJOR_VERSION >= 3
// The arguments of this structure tell Python what to call your extension,
// what it's methods are and where to look for it's method definitions
static struct PyModuleDef swclient_definition = {
    PyModuleDef_HEAD_INIT,
    "swclient",
    "A Python module that wraps swupdate client APIs.",
    -1,
    swclient_methods
};

PyMODINIT_FUNC PyInit_swclient(void) {
    Py_Initialize();
    return PyModule_Create(&swclient_definition);
}
#else
PyMODINIT_FUNC initswclient() {
    Py_InitModule3("swclient", swclient_methods, "A Python module that wraps swupdate client APIs.");
}
#endif
