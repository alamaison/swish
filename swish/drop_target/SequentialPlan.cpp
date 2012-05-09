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
#include "swish/drop_target/Operation.hpp"

#include <boost/bind.hpp>
#include <boost/cstdint.hpp> // int64_t
#include <boost/lambda/lambda.hpp> // _1, _2
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <comet/error.h> // com_error

#include <functional> // unary_function

using comet::com_error;
using comet::com_ptr;

using boost::bind;
using boost::int64_t;
using boost::numeric_cast;

using std::size_t;
using std::unary_function;

namespace swish {
namespace drop_target {

namespace {

	/**
	 * Functor handles 'intra-file' progress.
	 * 
	 * In other words, it handles the small increments that happen during the
	 * upload of one file amongst many.  We need this to give meaningful
	 * progress when only a small number of files are being dropped.
	 *
	 * @bug  Vulnerable to integer overflow with a very large number of files.
	 */
	class ProgressMicroUpdater : public unary_function<int, void>
	{
	public:
		ProgressMicroUpdater(
			Progress& progress, size_t current_file_index, size_t total_files)
			: m_progress(progress), m_current_file_index(current_file_index),
			  m_total_files(total_files)
		{}

		void operator()(int percent_done)
		{
			unsigned long long pos =
				(m_current_file_index * 100) + percent_done;
			m_progress.update(pos, m_total_files * 100);
		}

	private:
		Progress& m_progress;
		size_t m_current_file_index;
		size_t m_total_files;
	};

	/**
	 * Calculate percentage.
	 *
	 * @bug  Throws if using ludicrously large sizes.
	 */
	int percentage(int64_t done, int64_t total)
	{
		if (total == 0)
			return 100;
		else
			return numeric_cast<int>((done * 100) / total);
	}

}

void SequentialPlan::execute_plan(
	Progress& progress, com_ptr<ISftpProvider> provider,
	com_ptr<ISftpConsumer> consumer, CopyCallback& callback) const
{
	for (unsigned int i = 0; i < m_copy_list.size(); ++i)
	{
		if (progress.user_cancelled())
			BOOST_THROW_EXCEPTION(com_error(E_ABORT));

		const Operation& operation = m_copy_list.at(i);

		progress.line_path(1, operation.title());
		progress.line_path(2, operation.description());

		ProgressMicroUpdater micro_updater(progress, i, m_copy_list.size());

		operation(
			bind(micro_updater, bind(percentage, _1, _2)),
			provider, consumer, callback);

		// We update here as well, fixing the progress to a file boundary,
		// as we don't completely trust the intra-file progress.  A stream
		// could have lied about its size messing up the count.  This
		// will override any such errors.

		progress.update(i, m_copy_list.size());
	}
}

void SequentialPlan::add_stage(const Operation& entry)
{
	m_copy_list.push_back(entry.clone());
}

}}
