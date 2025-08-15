#!/usr/bin/env python3

import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as gc

from typing import List
import sys
import pprint
pp = pprint.PrettyPrinter(indent=4)

########################################

def check_type(var_, type_):
    if not isinstance(var_, type_):
        tn = type_.__name__
        vt = type(var_).__name__
        print(f"BF RUNTIME EXCEPTION: Expected {tn} type, but got {vt}.")
        # sys.exit(0)
        raise gc.BfruntimeRpcException("Type mismatch")

########################################

class Key():
    def __init__(self, key_, val_=None):
        check_type(key_, str)
        self._key = key_
        self._val = val_
    def key(self):
        return gc.KeyTuple(self._key, self._val)

class BoolKey(Key):
    def __init__(self, key_, val_):
        check_type(val_, bool)
        super().__init__(key_, val_)

class ExactKey(Key):
    def __init__(self, key_, val_):
        check_type(val_, int)
        super().__init__(key_, val_)

class TernaryKey(Key):
    def __init__(self, key_, val_, mask_):
        check_type(val_, int)
        check_type(mask_, int)
        super().__init__(key_, val_)
        self._mask = mask_
    def key(self):
        return gc.KeyTuple(self._key, value=self._val, mask=self._mask)

class LPMKey(Key):
    def __init__(self, key_, val_, mask_):
        check_type(val_, int)
        check_type(mask_, int)
        super().__init__(key_, val_)
        self._mask = mask_
    def key(self):
        return gc.KeyTuple(self._key, value=self._val, mask=self._mask)

class RangeKey(Key):
    def __init__(self, key_: str, low_: int, high_: int):
        check_type(low_, int)
        check_type(high_, int)
        super().__init__(key_)
        self._low  = low_
        self._high = high_
    def key(self):
        return gc.KeyTuple(self._key, low=self._low, high=self._high)

########################################

class Data():
    def __init__(self, key_, val_):
        check_type(key_, str)
        self._key = key_
        self._val = val_
    def data(self):
        return gc.DataTuple(self._key, self._val)

class IntData(Data):
    def __init__(self, key_, val_):
        check_type(val_, int)
        super().__init__(key_, val_)

class BoolData(Data):
    def __init__(self, key_, val_):
        check_type(val_, bool)
        super().__init__(key_, val_)
    def data(self):
        return gc.DataTuple(self._key, bool_val=self._val)

class StrData(Data):
    def __init__(self, key_, val_):
        check_type(val_, str)
        super().__init__(key_, val_)
    def data(self):
        return gc.DataTuple(self._key, str_val=self._val)

class BytesData(Data):
    def __init__(self, key_, val_):
        check_type(val_, bytearray)
        super().__init__(key_, val_)

class FetchData(Data):
    def __init__(self, key_):
        super().__init__(key_, None)
    def data(self):
        return gc.DataTuple(self._key)

class IntArrayData(Data):
    def __init__(self, key_, val_):
        check_type(val_, list)
        for v in val_: check_type(v, int)
        super().__init__(key_, val_)
    def data(self):
        return gc.DataTuple(self._key, int_arr_val=self._val)

class BoolArrayData(Data):
    def __init__(self, key_, val_):
        check_type(val_, list)
        for v in val_: check_type(v, bool)
        super().__init__(key_, val_)
    def data(self):
        return gc.DataTuple(self._key, bool_arr_val=self._val)

########################################

class Match:
    def __init__(self, keys_):
        for k in keys_: check_type(k, Key)
        self._keys = keys_
    def match(self):
        return [k.key() for k in self._keys]

class Action:
    def __init__(self, data_, func_=None):
        for d in data_: check_type(d, Data)
        if func_ is not None: check_type(func_, str)
        self._data = data_
        self._func = func_
    def action(self):
        return [d.data() for d in self._data]
    def func(self):
        return self._func

########################################

class BFRuntimeAgent:

    ########################################

    def __init__(
            self,
            p4_name=None,
            host="localhost",
            port=50052,
            client_id=0,
            device_id=0,
            pipe_id=0xffff,
            verbose=False
        ):
        """
        Build bfrt interface with the P4 program.
        """
        self.verbose = verbose
        grpc_addr = f"{host}:{port}"
        self.interface = gc.ClientInterface(grpc_addr=grpc_addr, client_id=client_id, device_id=device_id)
        self.print_log("Connected to the BF Runtime Server.")
        self.bfrt_info = self.interface.bfrt_info_get()
        if p4_name is None: p4_name = self.bfrt_info.p4_name_get()
        self.interface.bind_pipeline_config(p4_name)
        self.print_log(f"Connected to the P4 program: {p4_name}")
        self.target = gc.Target(device_id=device_id, pipe_id=pipe_id)
        self.tables = {}

    ########################################

    def exit(self):
        """
        Exit the BF runtime agent.
        """
        self.print_log("Exiting the BF runtime agent")
        # self.interface._tear_down_stream()
    
    ########################################

    def print_log(self, msg):
        """
        Pretty print a log message.
        """
        if self.verbose:
            indicator_str = "Barefoot runtime agent log:"
            log_msg       = f"\n{indicator_str}\n{msg}"
            print(log_msg)

    ########################################

    def load_table(self, table_name):
        """
        Load a match-action table.
        """
        if table_name in self.tables:
            self.print_log(f"ERROR: Could not load table: {table_name}; already exists.")
            sys.exit(1)
        try:
            self.tables[table_name] = self.bfrt_info.table_get(table_name)
            self.print_log(f"Loaded table: {table_name}")
        except gc.BfruntimeRpcException as e:
            self.print_log(f"BF RUNTIME EXCEPTION: Could not load table: {table_name}.")
            raise e

    ########################################

    def print_table(self, table_name):
        """
        Print the entries of a match-action table.
        """
        self.print_log(f"Table name: {table_name}; Entries:")
        table = self.tables[table_name]
        try:
            entries = table.entry_get(self.target)
            for i, (data, key) in enumerate(entries):
                print(f"Entry #{i}: {key} -> {data}")
        except gc.BfruntimeRpcException as e:
            self.print_log(f"BF RUNTIME EXCEPTION: Could not print entries of table: {table_name}.")
            raise e

    ########################################

    def flush_table(self, table_name):
        """
        Flush match-action table.
        """
        try:
            keys = []
            table = self.tables[table_name]
            entries = table.entry_get(self.target)
            for _, key in entries: keys.append(key)
            if len(keys) > 0: table.entry_del(self.target, keys)
            self.print_log(f"Flushed entries of table: {table_name}")
        except gc.BfruntimeRpcException as e:
            self.print_log(f"BF RUNTIME EXCEPTION: Could not flush table: {table_name}.")
            raise e

    ########################################

    def add_entry(self, table_name: str, match: Match, action: Action):
        """
        Add an entry to a match-action table.
        """
        table = self.tables[table_name]
        try:
            match_on = [table.make_key(match.match())]
            if action.func() is None:
                act_on = [table.make_data(action.action())]
            else:
                act_on = [table.make_data(action.action(), action.func())]
            table.entry_add (
                self.target, match_on, act_on
            )
            self.print_log(f"Added entry to table: {table_name}")
        except Exception as e:
            self.print_log(f"BF RUNTIME EXCEPTION: Could not add entry to table: {table_name}.")
            raise e

    ########################################

    def mod_entry(self, table_name: str, match: Match, action: Action):
        """
        Update an entry in a match-action table.
        """
        table = self.tables[table_name]
        try:
            match_on = [table.make_key(match.match())]
            if action.func() is None:
                act_on = [table.make_data(action.action())]
            else:
                act_on = [table.make_data(action.action(), action.func())]
            table.entry_mod (
                self.target, match_on, act_on
            )
            self.print_log(f"Updated table entry: {table_name}")
        except Exception as e:
            self.print_log(f"BF RUNTIME EXCEPTION: Could not update entry for table: {table_name}.")
            raise e

    ########################################

    def del_entry(self, table_name: str, match: Match):
        """
        Delete an entry from a match-action table.
        """
        table = self.tables[table_name]
        try:
            match_on = [table.make_key(match.match())]
            table.entry_del (
                self.target, match_on
            )
            self.print_log(f"Deleted table entry: {table_name}")
        except Exception as e:
            self.print_log(f"BF RUNTIME EXCEPTION: Could not delete entry from table: {table_name}.")
            raise e

    ########################################

    def get_entry(self, table_name: str, match: Match):
        """
        Get an entry from a match-action table.
        """
        table = self.tables[table_name]
        try:
            match_on = [table.make_key(match.match())]
            response = table.entry_get (
                self.target, match_on
            )
            response = next(response)
            key = None
            data = None
            if type(response[0]) == gc._Data and type(response[1]) == gc._Key:
                key = response[1]
                data = response[0]
            elif type(response[0]) == gc._Key and type(response[1]) == gc._Data:
                key = response[0]
                data = response[1]
            self.print_log(f"Read entry from table: {table_name}")
        except gc.BfruntimeRpcException as e:
            self.print_log(f"BF RUNTIME EXCEPTION: Could not read entry from table: {table_name}.")
            raise e
        return key.to_dict(), data.to_dict()

    ########################################
    