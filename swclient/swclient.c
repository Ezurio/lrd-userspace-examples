/*
 * (C) Copyright 2019
 *
 * SPDX-License-Identifier:     GPL-2.0-or-later
 */

/*
 * The library wraps swupdate client APIs
 */
#include <sys/socket.h>
#include <sys/select.h>
#include <progress_ipc.h>
#include <network_ipc.h>
#include <stdio.h>

#include <Python.h>

static int fd = -1;
static int dryrun = 1;

static PyObject * prepare_fw_update(PyObject *self, PyObject *args)
{
	struct swupdate_request req;
	const char* software_set;
	const char* running_mode;

	if (!PyArg_ParseTuple(args, "i", &dryrun))
	if (!PyArg_ParseTuple(args, "iss", &dryrun, &software_set, &running_mode)) {
		fprintf(stderr, "prepare_fw_update: PyArg_ParseTuple failed\n");
		return NULL;
	}

	if (fd > 0)
		ipc_end(fd);

	swupdate_prepare_req(&req);
	req.dry_run = dryrun ? RUN_DRYRUN : RUN_INSTALL;
	if (software_set && strlen(software_set))
		strncpy(req.software_set, software_set, sizeof(req.software_set) - 1);
	if (running_mode && strlen(running_mode))
		strncpy(req.running_mode, running_mode, sizeof(req.running_mode) - 1);

	fd = ipc_inst_start_ext(&req, sizeof(req));

	return Py_BuildValue("i", fd);
}

static PyObject * do_fw_update(PyObject *self, PyObject *args)
{
	int rc = -1;
	Py_buffer py_buf;

	if (!PyArg_ParseTuple(args, "y*", &py_buf)) {
		fprintf(stderr, "do_fw_update: PyArg_ParseTuple failed\n");
		return NULL;
	}

	rc = ipc_send_data(fd, py_buf.buf, py_buf.len);

	PyBuffer_Release(&py_buf);

	return Py_BuildValue("i", rc);
}

static PyObject * end_fw_update(PyObject *self, PyObject *Py_UNUSED(ignored))
{
	ipc_end(fd);
	fd = -1;
	Py_RETURN_NONE;
}

static PyObject * get_fw_update_state(PyObject *self, PyObject *Py_UNUSED(ignored))
{
	int rc = 0, valid = 0;
	struct progress_msg msg;
	fd_set rd_set;
	struct timeval timeout;

	int msg_fd = progress_ipc_connect(false);

	if (msg_fd < 0)
		Py_RETURN_NONE;

	for (;;) {
		FD_ZERO(&rd_set);
		FD_SET(msg_fd, &rd_set);

		/* Initialize the timeout data structure. */
		timeout.tv_sec = 0;
		timeout.tv_usec = 200000;

		rc = select(msg_fd + 1, &rd_set, NULL, NULL, &timeout);
		if (rc <= 0)
			break;

		read(msg_fd, &msg, sizeof(msg));
		valid = 1;
	}

	ipc_end(msg_fd);
	if (!valid)
		Py_RETURN_NONE;

	/* Build the output tuple */
	return Py_BuildValue("iIIIss", msg.status, msg.nsteps, msg.cur_step,
			     msg.cur_percent, msg.cur_image, msg.info);
}

static PyMethodDef swclient_methods[] =
{
	{ "prepare_fw_update",	 prepare_fw_update,   METH_VARARGS, "Prepare to update firmware"	 },
	{ "do_fw_update",	 do_fw_update,	      METH_VARARGS, "Do firmware update"		 },
	{ "end_fw_update",	 end_fw_update,	      METH_NOARGS,  "End firmware update"		 },
	{ "get_fw_update_state", get_fw_update_state, METH_NOARGS,  "Get firmware update progress state" },
	{ NULL,			 NULL,		      0,	    NULL				 }
};

// The arguments of this structure tell Python what to call your extension,
// what it's methods are and where to look for it's method definitions
static struct PyModuleDef swclient_definition =
{
	PyModuleDef_HEAD_INIT,
	"swclient",
	"A Python module that wraps swupdate client APIs.",
	-1,
	swclient_methods
};

PyMODINIT_FUNC PyInit_swclient(void)
{
	Py_Initialize();
	return PyModule_Create(&swclient_definition);
}
