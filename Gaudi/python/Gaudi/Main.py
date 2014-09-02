import sys, os
from time import time
from Gaudi import Configuration
import logging

log = logging.getLogger(__name__)

def toOpt(value):
    '''
    Helper to convert values to old .opts format.

    >>> print toOpt('some "text"')
    "some \\"text\\""
    >>> print toOpt('first\\nsecond')
    "first
    second"
    >>> print toOpt({'a': [1, 2, '3']})
    {"a": [1, 2, "3"]}
    '''
    if isinstance(value, basestring):
        return '"{0}"'.format(value.replace('"', '\\"'))
    elif isinstance(value, dict):
        return '{{{0}}}'.format(', '.join('{0}: {1}'.format(toOpt(k), toOpt(v))
                                          for k, v in value.iteritems()))
    elif hasattr(value, '__iter__'):
        return '[{0}]'.format(', '.join(map(toOpt, value)))
    else:
        return repr(value)

class gaudimain(object) :
    # main loop implementation, None stands for the default
    mainLoop = None

    def __init__(self) :
        from Configurables import ApplicationMgr
        appMgr = ApplicationMgr()
        if "GAUDIAPPNAME" in os.environ:
            appMgr.AppName = str(os.environ["GAUDIAPPNAME"])
        if "GAUDIAPPVERSION" in os.environ:
            appMgr.AppVersion = str(os.environ["GAUDIAPPVERSION"])
        self.log = logging.getLogger(__name__)
        self.printsequence = False

    def setupParallelLogging( self ) :
        # ---------------------------------------------------
        # set up Logging
        # ----------------
        # from multiprocessing import enableLogging, getLogger
        import multiprocessing
        # preliminaries for handlers/output files, etc.
        from time import ctime
        datetime = ctime()
        datetime = datetime.replace(' ', '_')
        outfile = open( 'gaudirun-%s.log'%(datetime), 'w' )
        # two handlers, one for a log file, one for terminal
        streamhandler = logging.StreamHandler(strm=outfile)
        console       = logging.StreamHandler()
        # create formatter : the params in parentheses are variable names available via logging
        formatter = logging.Formatter( "%(asctime)s - %(name)s - %(levelname)s - %(message)s" )
        # add formatter to Handler
        streamhandler.setFormatter(formatter)
        console.setFormatter(formatter)
        # now, configure the logger
        # enableLogging( level=0 )
        # self.log = getLogger()
        self.log = multiprocessing.log_to_stderr()
        self.log.setLevel( logging.INFO )
        self.log.name = 'Gaudi/Main.py Logger'
        self.log.handlers = []
        # add handlers to logger : one for output to a file, one for console output
        self.log.addHandler(streamhandler)
        self.log.addHandler(console)
        self.log.removeHandler(console)
        # set level!!
        self.log.setLevel = logging.INFO
        # ---------------------------------------------------

    def generatePyOutput(self, all = False):
        from pprint import pformat
        conf_dict = Configuration.configurationDict(all)
        return pformat(conf_dict)

    def generateOptsOutput(self, all = False):
        from pprint import pformat
        conf_dict = Configuration.configurationDict(all)
        out = []
        names = conf_dict.keys()
        names.sort()
        for n in names:
            props = conf_dict[n].keys()
            props.sort()
            for p in props:
                out.append('%s.%s = %s;' % (n,p, toOpt(conf_dict[n][p])))
        return "\n".join(out)

    def _writepickle(self, filename) :
        #--- Lets take the first file input file as the name of the pickle file
        import pickle
        output = open(filename, 'wb')
        # Dump only the the configurables that make sense to dump (not User ones)
        from GaudiKernel.Proxy.Configurable import getNeededConfigurables
        to_dump = {}
        for n in getNeededConfigurables():
            to_dump[n] = Configuration.allConfigurables[n]
        pickle.dump(to_dump, output, -1)
        output.close()

    def printconfig(self, old_format = False, all = False) :
        msg = 'Dumping all configurables and properties'
        if not all:
            msg += ' (different from default)'
        log.info(msg)
        conf_dict = Configuration.configurationDict(all)
        if old_format:
            print self.generateOptsOutput(all)
        else:
            print self.generatePyOutput(all)

    def writeconfig(self, filename, all = False):
        write = { ".pkl" : lambda filename, all: self._writepickle(filename),
                  ".py"  : lambda filename, all: open(filename,"w").write(self.generatePyOutput(all) + "\n"),
                  ".opts": lambda filename, all: open(filename,"w").write(self.generateOptsOutput(all) + "\n"),
                }
        from os.path import splitext
        ext = splitext(filename)[1]
        if ext in write:
            write[ext](filename, all)
        else:
            log.error("Unknown file type '%s'. Must be any of %r.", ext, write.keys())
            sys.exit(1)

    def _printsequence(self):
        if not self.printsequence:
            # No printing requested
            return

        def printAlgo( algName, appMgr, prefix = ' ') :
            print prefix + algName
            alg = appMgr.algorithm( algName.split( "/" )[ -1 ] )
            prop = alg.properties()
            if prop.has_key( "Members" ) :
                subs = prop[ "Members" ].value()
                for i in subs : printAlgo( i.strip( '"' ), appMgr, prefix + "     " )
            elif prop.has_key( "DetectorList" ) :
                subs = prop[ "DetectorList" ].value()
                for i in subs : printAlgo( algName.split( "/" )[ -1 ] + i.strip( '"' ) + "Seq", appMgr, prefix + "     ")

        mp = self.g.properties()
        print "\n ****************************** Algorithm Sequence **************************** \n"
        for i in mp["TopAlg"].value(): printAlgo( i, self.g )
        print "\n ****************************************************************************** \n"

    ## Instantiate and run the application.
    #  Depending on the number of CPUs (ncpus) specified, it start
    def run(self, ncpus = None):
        if not ncpus:
            # Standard sequential mode
            result = self.runSerial()
        else:
            # Otherwise, run with the specified number of cpus
            result = self.runParallel(ncpus)
        return result

    def basicInit(self):
        '''
        Bootstrap the application with minimal use of Python bindings.
        '''
        import cppyy
        import GaudiKernel.Proxy.Configurable
        if hasattr(GaudiKernel.Proxy.Configurable, "applyConfigurableUsers"):
            GaudiKernel.Proxy.Configurable.applyConfigurableUsers()

        try:
            from GaudiKernel.Proxy.Configurable import expandvars
        except ImportError:
            # pass-through implementation if expandvars is not defined (AthenaCommon)
            expandvars = lambda data : data

        from GaudiKernel.Proxy.Configurable import Configurable, getNeededConfigurables

        cppyy.gbl.DataObject
        self.g = cppyy.gbl.Gaudi.createApplicationMgr()

        ip = cppyy.libPyROOT.MakeNullPointer(cppyy.gbl.IProperty)
        if self.g.queryInterface(cppyy.gbl.IProperty.interfaceID(), ip).isFailure():
            self.log.error('Cannot get IProperty interface of ApplicationMgr')
            sys.exit(10)
        self.ip = ip

        # set ApplicationMgr properties
        comp = 'ApplicationMgr'
        props = Configurable.allConfigurables.get(comp, {})
        if props:
            props = expandvars(props.getValuedProperties())
        for p, v in props.items() + [('JobOptionsType', 'NONE')]:
            if not ip.setProperty(p, str(v)).isSuccess():
                self.log.error('Cannot set property %s.%s to %s', comp, p, v)
                sys.exit(10)
        self.g.configure()

        svcloc = cppyy.libPyROOT.MakeNullPointer(cppyy.gbl.ISvcLocator)
        if self.g.queryInterface(cppyy.gbl.ISvcLocator.interfaceID(), svcloc).isFailure():
            self.log.error('Cannot get ISvcLocator interface of ApplicationMgr')
            sys.exit(10)

        # set MessageSvc properties
        comp = 'MessageSvc'
        ms = cppyy.gbl.GaudiPython.Helper.service(svcloc, comp)
        msp = cppyy.libPyROOT.MakeNullPointer(cppyy.gbl.IProperty)
        if ms.queryInterface(cppyy.gbl.IProperty.interfaceID(), msp).isFailure():
            self.log.error('Cannot get IProperty interface of %s', comp)
            sys.exit(10)
        props = Configurable.allConfigurables.get(comp, {})
        if props:
            props = expandvars(props.getValuedProperties())
        for p, v in props.items():
            if not msp.setProperty(p, str(v)).isSuccess():
                self.log.error('Cannot set property %s.%s to %s', comp, p, v)
                sys.exit(10)

        # feed JobOptionsSvc
        comp = 'JobOptionsSvc'
        jos = cppyy.gbl.GaudiPython.Helper.service(svcloc, comp)
        josp = cppyy.libPyROOT.MakeNullPointer(cppyy.gbl.IJobOptionsSvc)
        if jos.queryInterface(cppyy.gbl.IJobOptionsSvc.interfaceID(), josp).isFailure():
            self.log.error('Cannot get IJobOptionsSvc interface of %s', comp)
            sys.exit(10)
        for n in getNeededConfigurables():
            c = Configurable.allConfigurables[n]
            if n in ['ApplicationMgr','MessageSvc']:
                continue # These are already done
            for p, v in  c.getValuedProperties().items() :
                v = expandvars(v)
                # Note: AthenaCommon.Configurable does not have Configurable.PropertyReference
                if hasattr(Configurable,"PropertyReference") and type(v) == Configurable.PropertyReference:
                    # this is done in "getFullName", but the exception is ignored,
                    # so we do it again to get it
                    v = v.__resolve__()
                if   type(v) == str : v = '"%s"' % v # need double quotes
                elif type(v) == long: v = '%d'   % v # prevent pending 'L'
                josp.addPropertyToCatalogue(n, cppyy.gbl.StringProperty(p,str(v)))
        if hasattr(Configurable,"_configurationLocked"):
            Configurable._configurationLocked = True

    def gaudiPythonInit(self):
        '''
        Initialize the application with full Python bindings.
        '''
        import GaudiPython
        self.g = GaudiPython.AppMgr()
        self.ip = self.g._ip

    def runSerial(self) :
        #--- Instantiate the ApplicationMgr------------------------------
        if self.printsequence or self.mainLoop:
            self.gaudiPythonInit()
        else:
            self.basicInit()

        self.log.debug('-'*80)
        self.log.debug('%s: running in serial mode', __name__)
        self.log.debug('-'*80)
        sysStart = time()

        self._printsequence()

        if self.mainLoop:
            runner = self.mainLoop
        else:
            def runner(app, nevt):
                import cppyy
                ep = cppyy.libPyROOT.MakeNullPointer(cppyy.gbl.IEventProcessor)
                if app.queryInterface(cppyy.gbl.IEventProcessor.interfaceID(), ep).isFailure():
                    self.log.error('Cannot get IEventProcessor')
                    return False
                self.log.debug('initialize')
                sc = app.initialize()
                if sc.isSuccess():
                    self.log.debug('start')
                    sc = app.start()
                    if sc.isSuccess():
                        self.log.debug('run(%d)', nevt)
                        sc = ep.executeRun(nevt)
                        self.log.debug('stop')
                        app.stop().ignore()
                    self.log.debug('finalize')
                    app.finalize().ignore()
                self.log.debug('terminate')
                sc1 = app.terminate()
                if sc.isSuccess():
                    sc = sc1
                else:
                    sc1.ignore()
                self.log.debug('status code: %s',
                               'SUCCESS' if sc.isSuccess() else 'FAILURE')
                return sc

        try:
            statuscode = runner(self.g, int(self.ip.getProperty('EvtMax').toString()))
        except SystemError:
            # It may not be 100% correct, but usually it means a segfault in C++
            self.ip.setProperty('ReturnCode', str(128 + 11))
            statuscode = False
        except Exception, x:
            print 'Exception:', x
            # for other exceptions, just set a generic error code
            self.ip.setProperty('ReturnCode', '1')
            statuscode = False
        if hasattr(statuscode, "isSuccess"):
            success = statuscode.isSuccess()
        else:
            success = statuscode
        #success = self.g.exit().isSuccess() and success
        if not success and self.ip.getProperty('ReturnCode').toString() == '0':
            # ensure that the return code is correctly set
            self.ip.setProperty('ReturnCode', '1')
        sysTime = time()-sysStart
        self.log.debug('-'*80)
        self.log.debug('%s: serial system finished, time taken: %5.4fs', __name__, sysTime)
        self.log.debug('-'*80)
        return int(self.ip.getProperty('ReturnCode').toString())

    def runParallel(self, ncpus) :
        if self.mainLoop:
            self.log.fatal("Cannot use custom main loop in multi-process mode, check your options")
            return 1
        self.setupParallelLogging( )
        from Gaudi.Configuration import Configurable
        import GaudiMP.GMPBase as gpp
        c = Configurable.allConfigurables
        self.log.info('-'*80)
        self.log.info('%s: Parallel Mode : %i '%(__name__, ncpus))
        from commands import getstatusoutput as gso
        metadataCommands = [ 'uname -a',
                             'echo $CMTCONFIG',
                             'echo $GAUDIAPPNAME',
                             'echo $GAUDIAPPVERSION']
        for comm in metadataCommands :
            s, o = gso( comm )
            if s :
                o = "Undetermined"
            string = '%s: %30s : %s '%(__name__, comm, o)
            self.log.info( string )
        try :
            events = str(c['ApplicationMgr'].EvtMax)
        except :
            events = "Undetermined"
        self.log.info('%s: Events Specified : %s '%(__name__,events))
        self.log.info('-'*80)
        # Parall = gpp.Coordinator(ncpus, shared, c, self.log)
        Parall = gpp.Coord( ncpus, c, self.log )
        sysStart = time()
        sc = Parall.Go()
        self.log.info('MAIN.PY : received %s from Coordinator'%(sc))
        if sc.isFailure() :
            return 1
        sysTime = time()-sysStart
        self.log.name = 'Gaudi/Main.py Logger'
        self.log.info('-'*80)
        self.log.info('%s: parallel system finished, time taken: %5.4fs', __name__, sysTime)
        self.log.info('-'*80)
        return 0
