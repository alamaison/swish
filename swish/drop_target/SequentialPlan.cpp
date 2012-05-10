/**
    @file

    Standard drop operation plan implementation.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    @endif
*/

#include "SequentialPlan.hpp"

#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer
#include "swish/drop_target/DropActionCallback.hpp"
#include "swish/drop_target/Operation.hpp"

#include <boost/cstdint.hpp> // uintmax_t
#include <boost/filesystem/path.hpp> // wpath
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <comet/error.h> // com_error

#include <cassert> // assert
#include <memory> // auto_ptr

using comet::com_error;
using comet::com_ptr;

using boost::filesystem::wpath;
using boost::numeric_cast;
using boost::uintmax_t;

using std::auto_ptr;
using std::size_t;

namespace swish {
namespace drop_target {

namespace {

	/**
	 * Calculate percentage.
	 *
	 * @bug  Throws if using ludicrously large sizes.
	 */
	uintmax_t percentage(uintmax_t done, uintmax_t total)
	{
		if (total == 0)
			return 100;
		else
			return numeric_cast<uintmax_t>((done * 100) / total);
	}

	/**
	 * Calculator of 'intra-file' progress.
	 *
	 * Purpose: to translate between the progress increments reported by
	 * a single operation and the overall progress of the sequence of
	 * operations.
	 *
	 * In other words, it handles the small increments that happen during the
	 * upload of one file amongst many.  We need this to give meaningful
	 * progress when only a small number of files are being dropped where the
	 * time spent on a single file makes up a significant portion of the
	 * overall transfer.
	 */
	class IntraSequenceCallback : public OperationCallback
	{
	public:

		IntraSequenceCallback(
			OperationCallback& sequence_callback, size_t current_file_index,
			size_t total_files)
			:
			m_callback(sequence_callback),
			m_current_file_index(current_file_index),
			m_total_files(total_files) {}

		virtual bool has_user_cancelled() const
		{
			return m_callback.has_user_cancelled();
		}

		virtual bool request_overwrite_permission(const wpath& target) const
		{
			return m_callback.request_overwrite_permission(target);
		}

		/**
		 * Update the overall sequence progress with the intra-operation
		 * progress.
		 *
		 * This uses of resolution of 100 update intervals per file in the
		 * sequence.  In other words, the intra-operation is converted to a
		 * percentage.
		 */
		virtual void update_progress(uintmax_t so_far, uintmax_t out_of)
		{
			uintmax_t percent_done = percentage(so_far, out_of);

			uintmax_t current = (m_current_file_index * 100) + percent_done;

			m_callback.update_progress(current, m_total_files * 100);
		}

	private:
		OperationCallback& m_callback;
		size_t m_current_file_index;
		const size_t m_total_files;
	};

	/**
	 * Executes one of a sequence of operations.
	 *
	 * Purpose: to liaise between the Operation and the DropActionCallback
	 * interface used to communicator with the user.
	 *
	 * The DropActionCallback creates and starts the progress dialogue when
	 * it is requested so part of that liaison is making sure this only
	 * happens once for the entire sequence of operations.
	 */
	class OperationExecutor : private OperationCallback
	{
	public:
		OperationExecutor(DropActionCallback& callback)
			: m_callback(callback) {}

		void operator()(
			const Operation& operation, size_t operation_index,
			size_t total_operations, com_ptr<ISftpProvider> provider,
			com_ptr<ISftpConsumer> consumer)
		{
			progress().line_path(1, operation.title());
			progress().line_path(2, operation.description());

			IntraSequenceCallback micro_updater(
				*this, operation_index, total_operations);

			if (has_user_cancelled())
			{
				BOOST_THROW_EXCEPTION(com_error(E_ABORT));
			}
			else
			{
				operation(micro_updater, provider, consumer);

				// We update here as well, fixing the progress to a file 
				// boundary, as we don't completely trust the intra-file
				// progress.  A stream could have lied about its size messing
				// up the count.  This will override any such errors.

				assert(operation_index + 1 <= total_operations);
				progress().update(operation_index + 1, total_operations);
			}
		}

		virtual bool has_user_cancelled() const
		{
			if (m_progress.get())
				return m_progress->user_cancelled();
			else
				return false;
		}

		virtual bool request_overwrite_permission(const wpath& target) const
		{
			return m_callback.can_overwrite(target);
		}

		virtual void update_progress(uintmax_t so_far, uintmax_t out_of)
		{
			progress().update(so_far, out_of);
		}

	private:

		Progress& progress()
		{
			if (!m_progress.get())
				m_progress = m_callback.progress();

			return *m_progress;
		}

		DropActionCallback& m_callback;
		auto_ptr<Progress> m_progress;
	};

}

void SequentialPlan::execute_plan(
	DropActionCallback& callback, com_ptr<ISftpProvider> provider,
	com_ptr<ISftpConsumer> consumer) const
{
	OperationExecutor executor(callback);

	for (unsigned int i = 0; i < m_copy_list.size(); ++i)
	{
		const Operation& operation = m_copy_list.at(i);
		executor(operation, i, m_copy_list.size(), provider, consumer);
	}
}

void SequentialPlan::add_stage(const Operation& entry)
{
	m_copy_list.push_back(entry.clone());
}

}}
