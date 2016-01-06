#-------------------------------------------------
#
# Project created by QtCreator 2016-01-06T02:10:16
#
#-------------------------------------------------

QT       -= core gui


TEMPLATE = lib

DEFINES += OPENVDB_TO_SOFTIMAGE_LIBRARY

DEFINES += OPENVDB_OPENEXR_STATICLIB
DEFINES += OPENVDB_STATICLIB
#DEFINES += OPENVDB_HAS_CXX11 ## <- maybe uncomment in case of gcc 4.9(or 4.8?) and up
DEFINES += BOOST_SYSTEM_NO_DEPRECATED
DEFINES += HALF_EXPORTS



INCLUDEPATH +=$$quote($$(XSISDK_ROOT)/include)
#INCLUDEPATH += $$quote($$(BOOST_INCL_DIR))
INCLUDEPATH += $$quote($$PWD/external)
INCLUDEPATH += $$quote($$PWD/external/tbb/include)
INCLUDEPATH += $$quote($$PWD/external/zlib)

SOURCES +=  debugNode.cpp \
    GetFileContentInfo.cpp \
    VDB_AdvectLevelSet.cpp \
    VDB_AdvectPoints.cpp \
    VDB_ConvertGridClass.cpp \
    VDB_CopyGrid.cpp \
    VDB_CreateGrid.cpp \
    VDB_CSG.cpp \
    VDB_FillBoxGrid.cpp \
    VDB_Fracture.cpp \
    VDB_GeometryQueries.cpp \
    VDB_GeometryToGrid.cpp \
    VDB_GetGridData.cpp \
    VDB_GetValueAtIndex.cpp \
    VDB_GetValueAtPosition.cpp \
    VDB_GridBooleanOp.cpp \
    VDB_GridIO.cpp \
    VDB_GridToSpheres.cpp \
    VDB_GridToTopology.cpp \
    VDB_Morpher.cpp \
    VDB_MorphologicalFilter.cpp \
    VDB_Noise.cpp \
    VDB_ParticlesToGrid.cpp \
    VDB_Platonic.cpp \
    VDB_RebuildLevelSet.cpp \
    VDB_ResampleGrid.cpp \
    VDB_SetValueAtPosition.cpp \
    VDB_Transform.cpp \
    main.cpp \
    external/boost/boost_fileio_impl.cpp \
    external/openvdb/cmd/openvdb_print/main.cc \
    external/openvdb/cmd/openvdb_render/main.cc \
    external/openvdb/cmd/openvdb_view/main.cc \
    external/openvdb/io/Archive.cc \
    external/openvdb/io/Compression.cc \
    external/openvdb/io/File.cc \
    external/openvdb/io/GridDescriptor.cc \
    external/openvdb/io/Queue.cc \
    external/openvdb/io/Stream.cc \
    external/openvdb/math/Hermite.cc \
    external/openvdb/math/Maps.cc \
    external/openvdb/math/Proximity.cc \
    external/openvdb/math/QuantizedUnitVec.cc \
    external/openvdb/math/Transform.cc \
    external/openvdb/metadata/Metadata.cc \
    external/openvdb/metadata/MetaMap.cc \
    external/openvdb/util/Formats.cc \
    external/openvdb/util/Util.cc \
    external/openvdb/Grid.cc \
    external/openvdb/openvdb.cc \
    external/openvdb/Platform.cc \
    external/zlib/adler32.c \
    external/zlib/compress.c \
    external/zlib/crc32.c \
    external/zlib/deflate.c \
    external/zlib/gzclose.c \
    external/zlib/gzlib.c \
    external/zlib/gzread.c \
    external/zlib/gzwrite.c \
    external/zlib/infback.c \
    external/zlib/inffast.c \
    external/zlib/inflate.c \
    external/zlib/inftrees.c \
    external/zlib/trees.c \
    external/zlib/uncompr.c \
    external/zlib/zutil.c


HEADERS += Main.h \
    MathUtils.h \
    noises.h \
    vdbHelpers.h \
    external/OpenEXR/half.h \
    external/openvdb/io/Archive.h \
    external/openvdb/io/Compression.h \
    external/openvdb/io/File.h \
    external/openvdb/io/GridDescriptor.h \
    external/openvdb/io/Queue.h \
    external/openvdb/io/Stream.h \
    external/openvdb/math/BBox.h \
    external/openvdb/math/Coord.h \
    external/openvdb/math/DDA.h \
    external/openvdb/math/FiniteDifference.h \
    external/openvdb/math/Hermite.h \
    external/openvdb/math/LegacyFrustum.h \
    external/openvdb/math/Maps.h \
    external/openvdb/math/Mat.h \
    external/openvdb/math/Mat3.h \
    external/openvdb/math/Mat4.h \
    external/openvdb/math/Math.h \
    external/openvdb/math/Operators.h \
    external/openvdb/math/Proximity.h \
    external/openvdb/math/QuantizedUnitVec.h \
    external/openvdb/math/Quat.h \
    external/openvdb/math/Ray.h \
    external/openvdb/math/Stats.h \
    external/openvdb/math/Stencils.h \
    external/openvdb/math/Transform.h \
    external/openvdb/math/Tuple.h \
    external/openvdb/math/Vec2.h \
    external/openvdb/math/Vec3.h \
    external/openvdb/math/Vec4.h \
    external/openvdb/metadata/Metadata.h \
    external/openvdb/metadata/MetaMap.h \
    external/openvdb/metadata/StringMetadata.h \
    external/openvdb/tools/Composite.h \
    external/openvdb/tools/Dense.h \
    external/openvdb/tools/DenseSparseTools.h \
    external/openvdb/tools/Filter.h \
    external/openvdb/tools/GridOperators.h \
    external/openvdb/tools/GridTransformer.h \
    external/openvdb/tools/Interpolation.h \
    external/openvdb/tools/LevelSetAdvect.h \
    external/openvdb/tools/LevelSetFilter.h \
    external/openvdb/tools/LevelSetFracture.h \
    external/openvdb/tools/LevelSetMeasure.h \
    external/openvdb/tools/LevelSetMorph.h \
    external/openvdb/tools/LevelSetRebuild.h \
    external/openvdb/tools/LevelSetSphere.h \
    external/openvdb/tools/LevelSetTracker.h \
    external/openvdb/tools/LevelSetUtil.h \
    external/openvdb/tools/MeshToVolume.h \
    external/openvdb/tools/Morphology.h \
    external/openvdb/tools/ParticlesToLevelSet.h \
    external/openvdb/tools/PointAdvect.h \
    external/openvdb/tools/PointScatter.h \
    external/openvdb/tools/RayIntersector.h \
    external/openvdb/tools/RayTracer.h \
    external/openvdb/tools/Statistics.h \
    external/openvdb/tools/ValueTransformer.h \
    external/openvdb/tools/VectorTransformer.h \
    external/openvdb/tools/VolumeToMesh.h \
    external/openvdb/tools/VolumeToSpheres.h \
    external/openvdb/tree/InternalNode.h \
    external/openvdb/tree/Iterator.h \
    external/openvdb/tree/LeafManager.h \
    external/openvdb/tree/LeafNode.h \
    external/openvdb/tree/LeafNodeBool.h \
    external/openvdb/tree/NodeUnion.h \
    external/openvdb/tree/RootNode.h \
    external/openvdb/tree/Tree.h \
    external/openvdb/tree/TreeIterator.h \
    external/openvdb/tree/Util.h \
    external/openvdb/tree/ValueAccessor.h \
    external/openvdb/util/Formats.h \
    external/openvdb/util/logging.h \
    external/openvdb/util/MapsUtil.h \
    external/openvdb/util/Name.h \
    external/openvdb/util/NodeMasks.h \
    external/openvdb/util/NullInterrupter.h \
    external/openvdb/util/Util.h \
    external/openvdb/Exceptions.h \
    external/openvdb/Grid.h \
    external/openvdb/Metadata.h \
    external/openvdb/openvdb.h \
    external/openvdb/Platform.h \
    external/openvdb/PlatformConfig.h \
    external/openvdb/Types.h \
    external/openvdb/version.h \
    external/tbb/include/serial/tbb/parallel_for.h \
    external/tbb/include/serial/tbb/tbb_annotate.h \
    external/tbb/include/tbb/compat/ppl.h \
    external/tbb/include/tbb/internal/_aggregator_impl.h \
    external/tbb/include/tbb/internal/_concurrent_queue_impl.h \
    external/tbb/include/tbb/internal/_concurrent_unordered_impl.h \
    external/tbb/include/tbb/internal/_flow_graph_impl.h \
    external/tbb/include/tbb/internal/_flow_graph_indexer_impl.h \
    external/tbb/include/tbb/internal/_flow_graph_item_buffer_impl.h \
    external/tbb/include/tbb/internal/_flow_graph_join_impl.h \
    external/tbb/include/tbb/internal/_flow_graph_node_impl.h \
    external/tbb/include/tbb/internal/_flow_graph_tagged_buffer_impl.h \
    external/tbb/include/tbb/internal/_flow_graph_trace_impl.h \
    external/tbb/include/tbb/internal/_flow_graph_types_impl.h \
    external/tbb/include/tbb/internal/_mutex_padding.h \
    external/tbb/include/tbb/internal/_range_iterator.h \
    external/tbb/include/tbb/internal/_tbb_hash_compare_impl.h \
    external/tbb/include/tbb/internal/_tbb_strings.h \
    external/tbb/include/tbb/internal/_tbb_windef.h \
    external/tbb/include/tbb/internal/_template_helpers.h \
    external/tbb/include/tbb/internal/_x86_eliding_mutex_impl.h \
    external/tbb/include/tbb/internal/_x86_rtm_rw_mutex_impl.h \
    external/tbb/include/tbb/machine/gcc_armv7.h \
    external/tbb/include/tbb/machine/gcc_generic.h \
    external/tbb/include/tbb/machine/gcc_ia32_common.h \
    external/tbb/include/tbb/machine/gcc_itsx.h \
    external/tbb/include/tbb/machine/ibm_aix51.h \
    external/tbb/include/tbb/machine/icc_generic.h \
    external/tbb/include/tbb/machine/linux_common.h \
    external/tbb/include/tbb/machine/linux_ia32.h \
    external/tbb/include/tbb/machine/linux_ia64.h \
    external/tbb/include/tbb/machine/linux_intel64.h \
    external/tbb/include/tbb/machine/mac_ppc.h \
    external/tbb/include/tbb/machine/macos_common.h \
    external/tbb/include/tbb/machine/mic_common.h \
    external/tbb/include/tbb/machine/msvc_armv7.h \
    external/tbb/include/tbb/machine/msvc_ia32_common.h \
    external/tbb/include/tbb/machine/sunos_sparc.h \
    external/tbb/include/tbb/machine/windows_api.h \
    external/tbb/include/tbb/machine/windows_ia32.h \
    external/tbb/include/tbb/machine/windows_intel64.h \
    external/tbb/include/tbb/machine/xbox360_ppc.h \
    external/tbb/include/tbb/aggregator.h \
    external/tbb/include/tbb/aligned_space.h \
    external/tbb/include/tbb/atomic.h \
    external/tbb/include/tbb/blocked_range.h \
    external/tbb/include/tbb/blocked_range2d.h \
    external/tbb/include/tbb/blocked_range3d.h \
    external/tbb/include/tbb/cache_aligned_allocator.h \
    external/tbb/include/tbb/combinable.h \
    external/tbb/include/tbb/concurrent_hash_map.h \
    external/tbb/include/tbb/concurrent_lru_cache.h \
    external/tbb/include/tbb/concurrent_priority_queue.h \
    external/tbb/include/tbb/concurrent_queue.h \
    external/tbb/include/tbb/concurrent_unordered_map.h \
    external/tbb/include/tbb/concurrent_unordered_set.h \
    external/tbb/include/tbb/concurrent_vector.h \
    external/tbb/include/tbb/critical_section.h \
    external/tbb/include/tbb/enumerable_thread_specific.h \
    external/tbb/include/tbb/flow_graph.h \
    external/tbb/include/tbb/flow_graph_opencl_node.h \
    external/tbb/include/tbb/global_control.h \
    external/tbb/include/tbb/memory_pool.h \
    external/tbb/include/tbb/mutex.h \
    external/tbb/include/tbb/null_mutex.h \
    external/tbb/include/tbb/null_rw_mutex.h \
    external/tbb/include/tbb/parallel_do.h \
    external/tbb/include/tbb/parallel_for.h \
    external/tbb/include/tbb/parallel_for_each.h \
    external/tbb/include/tbb/parallel_invoke.h \
    external/tbb/include/tbb/parallel_reduce.h \
    external/tbb/include/tbb/parallel_scan.h \
    external/tbb/include/tbb/parallel_sort.h \
    external/tbb/include/tbb/parallel_while.h \
    external/tbb/include/tbb/partitioner.h \
    external/tbb/include/tbb/pipeline.h \
    external/tbb/include/tbb/queuing_mutex.h \
    external/tbb/include/tbb/queuing_rw_mutex.h \
    external/tbb/include/tbb/reader_writer_lock.h \
    external/tbb/include/tbb/recursive_mutex.h \
    external/tbb/include/tbb/runtime_loader.h \
    external/tbb/include/tbb/scalable_allocator.h \
    external/tbb/include/tbb/spin_mutex.h \
    external/tbb/include/tbb/spin_rw_mutex.h \
    external/tbb/include/tbb/task.h \
    external/tbb/include/tbb/task_arena.h \
    external/tbb/include/tbb/task_group.h \
    external/tbb/include/tbb/task_scheduler_init.h \
    external/tbb/include/tbb/task_scheduler_observer.h \
    external/tbb/include/tbb/tbb.h \
    external/tbb/include/tbb/tbb_allocator.h \
    external/tbb/include/tbb/tbb_config.h \
    external/tbb/include/tbb/tbb_exception.h \
    external/tbb/include/tbb/tbb_machine.h \
    external/tbb/include/tbb/tbb_profiling.h \
    external/tbb/include/tbb/tbb_stddef.h \
    external/tbb/include/tbb/tbb_thread.h \
    external/tbb/include/tbb/tbbmalloc_proxy.h \
    external/tbb/include/tbb/tick_count.h \
    external/tbb/src/old/concurrent_queue_v2.h \
    external/tbb/src/old/concurrent_vector_v2.h \
    external/tbb/src/old/spin_rw_mutex_v2.h \
    external/tbb/src/perf/coarse_grained_raii_lru_cache.h \
    external/tbb/src/perf/perf.h \
    external/tbb/src/perf/statistics.h \
    external/tbb/src/perf/statistics_xml.h \
    external/tbb/src/perf/time_framework.h \
    external/tbb/src/perf/time_sandbox.h \
    external/tbb/src/rml/client/library_assert.h \
    external/tbb/src/rml/client/omp_dynamic_link.h \
    external/tbb/src/rml/client/rml_factory.h \
    external/tbb/src/rml/include/rml_base.h \
    external/tbb/src/rml/include/rml_omp.h \
    external/tbb/src/rml/include/rml_tbb.h \
    external/tbb/src/rml/perfor/thread_level.h \
    external/tbb/src/rml/server/job_automaton.h \
    external/tbb/src/rml/server/thread_monitor.h \
    external/tbb/src/rml/server/wait_counter.h \
    external/tbb/src/rml/test/test_server.h \
    external/tbb/src/tbb/tools_api/internal/ittnotify.h \
    external/tbb/src/tbb/tools_api/legacy/ittnotify.h \
    external/tbb/src/tbb/tools_api/prototype/ittnotify.h \
    external/tbb/src/tbb/tools_api/disable_warnings.h \
    external/tbb/src/tbb/tools_api/ittnotify.h \
    external/tbb/src/tbb/tools_api/ittnotify_config.h \
    external/tbb/src/tbb/tools_api/ittnotify_static.h \
    external/tbb/src/tbb/tools_api/ittnotify_types.h \
    external/tbb/src/tbb/arena.h \
    external/tbb/src/tbb/cilk-tbb-interop.h \
    external/tbb/src/tbb/concurrent_monitor.h \
    external/tbb/src/tbb/custom_scheduler.h \
    external/tbb/src/tbb/dynamic_link.h \
    external/tbb/src/tbb/governor.h \
    external/tbb/src/tbb/intrusive_list.h \
    external/tbb/src/tbb/itt_notify.h \
    external/tbb/src/tbb/mailbox.h \
    external/tbb/src/tbb/market.h \
    external/tbb/src/tbb/observer_proxy.h \
    external/tbb/src/tbb/scheduler.h \
    external/tbb/src/tbb/scheduler_common.h \
    external/tbb/src/tbb/scheduler_utility.h \
    external/tbb/src/tbb/semaphore.h \
    external/tbb/src/tbb/task_stream.h \
    external/tbb/src/tbb/tbb_assert_impl.h \
    external/tbb/src/tbb/tbb_main.h \
    external/tbb/src/tbb/tbb_misc.h \
    external/tbb/src/tbb/tbb_statistics.h \
    external/tbb/src/tbb/tbb_version.h \
    external/tbb/src/tbb/tls.h \
    external/tbb/src/tbbmalloc/Customize.h \
    external/tbb/src/tbbmalloc/MapMemory.h \
    external/tbb/src/tbbmalloc/proxy.h \
    external/tbb/src/tbbmalloc/proxy_overload_osx.h \
    external/tbb/src/tbbmalloc/shared_utils.h \
    external/tbb/src/tbbmalloc/Statistics.h \
    external/tbb/src/tbbmalloc/tbb_function_replacement.h \
    external/tbb/src/tbbmalloc/tbbmalloc_internal.h \
    external/tbb/src/tbbmalloc/tbbmalloc_internal_api.h \
    external/tbb/src/tbbmalloc/TypeDefinitions.h \
    external/zlib/crc32.h \
    external/zlib/deflate.h \
    external/zlib/gzguts.h \
    external/zlib/inffast.h \
    external/zlib/inffixed.h \
    external/zlib/inflate.h \
    external/zlib/inftrees.h \
    external/zlib/trees.h \
    external/zlib/zconf.h \
    external/zlib/zlib.h \
    external/zlib/zutil.h


DESTDIR = ./build/XSI_OPENVDB_Workgroup/Application/Plugins/bin

win32 {
TARGET = /nt-x86-64/SoftimageOpenVDB

LIBS  +=$$quote($$(XSISDK_ROOT)/lib/nt-x86-64/sicoresdk.lib)
LIBS  +=$$quote($$(XSISDK_ROOT)/lib/nt-x86-64/sicppsdk.lib)

## tbb vc version is 10,
## so dll is also relies on that runtime redistributable(which should be installed anyway if you have xsi installed)
QMAKE_LIBDIR += $$quote($$PWD/external/tbb/lib)

DEFINES += NOMINMAX
QMAKE_CXXFLAGS += -bigobj

QMAKE_CXXFLAGS_RELEASE += /O2 /Oi /Ot /GS- /fp:fast
}


unix {
QMAKE_CXXFLAGS += -std=c++11
#QMAKE_LFLAGS += -Wl,-rpath='./'
TARGET = /SoftimageOpenVDB
DEFINES +=unix
DEFINES +=linux

QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3
}

build_pass:CONFIG(debug, debug|release) {
message("debug build selected")
DEFINES += _DEBUG
}

build_pass:CONFIG(release, debug|release) {
message("release build selected")
QMAKE_CFLAGS += -s
QMAKE_CXXFLAGS += -s
QMAKE_LFLAGS += -s
QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden

}

#$$VAR => QMake variable's value at the time qmake is run
#$${VAR} => QMake variable's value at time qmake is run (subtle difference)
#$(VAR) => Contents of an Environment variable at the time Makefile (not qmake) is run
