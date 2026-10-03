#ifndef __WP_ObjectTracker_H_
#define __WP_ObjectTracker_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/List.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>
#include <thread>
#include <vector>
#include <map>
#include <mutex>

namespace workphone
{
    /**
     * @file
     * @brief Tracks references to reference-counted objects and provides diagnostic dumps.
     *
     * The SharedObjectTracker collects records about objects that implement
     * the ISharedObject interface and the places in code that hold references
     * to those objects. It is intended to assist debugging reference-counting
     * issues (leaks, dangling references, unexpected owners).
     *
     * The tracker stores per-object metadata (source location where the object
     * was created) and per-reference metadata (address of the holder, call-site
     * file/line/function and an optional stack string).
     *
     * The tracker is accessed as a singleton via SharedObjectTracker::instance().
     */
    class WPCore_API SharedObjectTracker
    {
    public:
        /**
         * @brief Per-reference metadata.
         *
         * Records where a particular reference to an object was created/observed.
         * The `address` field typically holds the address of the pointer or holder
         * that keeps the reference (can be used to correlate with owner objects).
         */
        struct ReferenceData
        {
            ReferenceData();

            /**
             * @brief Construct reference metadata.
             * @param address Address of the reference holder (owner pointer, container slot, etc).
             * @param file Source file where the reference was recorded.
             * @param line Source line.
             * @param func Source function name.
             * @param s Optional stack or additional string info (stack trace).
             */
            ReferenceData( void *address, const char *file, u32 line, const char *func,
                           const String &s );

            /// Address of the holder that owns/holds this reference (may be nullptr).
            void *address = nullptr;

            /// Source line where the reference was recorded.
            u32 sourceLine = 0;

            /// Source file where the reference was recorded.
            String sourceFile;

            /// Source function where the reference was recorded.
            String sourceFunc;

            /// Optional stack trace or other diagnostic string captured at record time.
            String stack;
        };

        /**
         * @brief Per-object metadata and the collection of its references.
         *
         * Stores the object pointer, creation/source location, and a map of all
         * ReferenceData records keyed by the reference holder address.
         */
        struct ObjectData
        {
            ObjectData();
            /**
             * @brief Construct object metadata.
             * @param address Address of the object instance.
             * @param file Source file where the object was recorded/created.
             * @param line Source line.
             * @param func Source function name.
             * @param s Optional stack or additional information (stack trace).
             */
            ObjectData( void *address, const char *file, u32 line, const char *func, const String &s );

            /// Pointer to the tracked ISharedObject instance.
            ISharedObject *object = nullptr;

            /// Raw address of the object instance.
            void *address = nullptr;

            /// Source line where the object was recorded.
            u32 sourceLine = 0;

            /// Source file where the object was recorded.
            String sourceFile;

            /// Source function where the object was recorded.
            String sourceFunc;

            /// Optional stack trace or diagnostic string captured when the object was recorded.
            String stack;

            /// Map of reference-holder-address -> ReferenceData for all known references to this object.
            Map<void *, ReferenceData> referenceRecords;
        };

        SharedObjectTracker();
        ~SharedObjectTracker();

        /**
         * @brief Dump a diagnostic report for a specific object to a file.
         * @param object The object to report on (may be nullptr).
         * @param filePath Destination file path for the report (text).
         *
         * The report contains the object's recorded creation site and all known
         * reference records. Useful to persist diagnostics for later inspection.
         */
        void dumpReport( ISharedObject *object, const String &filePath );

        /**
         * @brief Dump a diagnostic report for a specific object to the default output.
         * @param object Raw pointer to the object.
         */
        void dumpReport( ISharedObject *object );

        /**
         * @brief Dump a diagnostic report for a specific object using a smart pointer.
         * @param object Smart pointer to the object.
         */
        void dumpReport( SmartPtr<ISharedObject> object );

        /**
         * @brief Dump diagnostic reports for all tracked objects.
         *
         * Prints or logs reports for every object currently recorded in the tracker.
         */
        void dumpReport();

        /**
         * @brief Retrieve the ObjectData for a tracked object.
         * @param object The object to find.
         * @return Pointer to the ObjectData if found, otherwise nullptr.
         *
         * Thread-safe for concurrent readers but callers should hold no assumptions
         * about the object's lifetime beyond the returned pointer; use the tracker
         * only for diagnostics.
         */
        ObjectData *getObjectData( ISharedObject *object );

        /**
         * @brief Record that a reference to @p object exists at @p address.
         * @param object The tracked object.
         * @param address Address of the reference holder.
         * @param file Source file where the reference was recorded.
         * @param line Source line.
         * @param func Source function name.
         *
         * If logging is enabled, captures and stores metadata so a dump can show
         * who holds references to the object.
         */
        void addReference( ISharedObject *object, void *address, const c8 *file, u32 line,
                           const c8 *func );

        /**
         * @brief Remove a previously recorded reference to an object.
         * @param object The tracked object.
         * @param address Address of the reference holder to remove.
         * @param file Source file (for diagnostic record of removal).
         * @param line Source line.
         * @param func Source function name.
         */
        void removeReference( ISharedObject *object, void *address, const c8 *file, u32 line,
                              const c8 *func );

        /**
         * @brief Add a top-level object record to the tracker.
         * @param object Object to add.
         *
         * Records the object so it shows up in dumps even if it has no active references.
         */
        void addObjectRecord( ISharedObject *object );

        /**
         * @brief Remove an object record from the tracker.
         * @param object Object to remove.
         *
         * Called when an object is destroyed or no longer needs tracking.
         */
        void removeObjectRecord( ISharedObject *object );

        /**
         * @brief Record creation/allocation of a raw object at a given address.
         * @param address Raw address of the new object.
         * @param file Source file where allocation/creation was recorded.
         * @param line Source line.
         * @param func Source function name.
         *
         * Typically used by debug instrumentation that cannot rely on an ISharedObject pointer.
         */
        void addObject( void *address, const c8 *file, u32 line, const c8 *func );

        /**
         * @brief Remove a raw object record for the given address.
         * @param address Raw address of the object to remove.
         */
        void removeObject( void *address );

        /**
         * @brief Dump a report describing all raw object records.
         *
         * This is a more general dump used when objects are tracked by address rather
         * than by ISharedObject interface pointer.
         */
        void dumpObjectReport();

        /**
         * @brief Access the singleton instance of the tracker.
         * @return Reference to the global SharedObjectTracker instance.
         *
         * The tracker is implemented as a process-global singleton for diagnostic purposes.
         */
        static SharedObjectTracker &instance();

    private:
        /**
         * @brief Background thread function used by the tracker.
         *
         * The concrete implementation may periodically perform maintenance tasks.
         * This is private and managed by the tracker instance.
         */
        static void threadFunc( void );

        ///
        /// Whether reference additions/removals should be logged. When false,
        /// addReference/removeReference are no-ops (or minimal).
        ///
        bool m_logReferences = true;

        ///
        /// Flag controlled atomically to indicate whether the tracker thread is running.
        ///
        atomic_bool m_isRunning = true;

        ///
        /// Optional thread used by the tracker for background tasks.
        ///
        std::thread *m_thread = nullptr;

        ///
        /// Map of object address -> ObjectData for all tracked objects.
        ///
        Map<void *, ObjectData> m_objectRecords;

        ///
        /// Mutex protecting access to m_objectRecords and internal state. Recursive
        /// because some public APIs may call into other tracked functions that also
        /// lock the tracker.
        ///
        mutable RecursiveMutex m_mutex;
    };
}  // namespace workphone

#endif
