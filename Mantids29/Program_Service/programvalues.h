#ifndef PROGRAMVALUES_H
#define PROGRAMVALUES_H

#include <string>
#include <list>

namespace Mantids29 { namespace Program { namespace Values {

/**
 * @brief The ProgramValues class contains information about a program.
 */
struct Author {
    std::string name; /**< Author name */
    std::string email; /**< Author email */
};

class ProgramValues {
public:
    /**
     * Class constructor.
     */
    ProgramValues();

    /**
     * Initializes the program name.
     * @param value Program name.
     */
    void initProgramName(const std::string &value);

    /**
     * Adds an author to the program.
     * @param author Author information.
     */
    void addAuthor(const Author & author);

    /**
     * Returns a list with all authors of the program.
     * @return List of authors.
     */
    std::list<Author> getAuthors();

    /**
     * Returns the program version.
     * @return Program version.
     */
    std::string getVersion() const;

    /**
     * Sets the program version.
     * @param value Program version.
     */
    void setVersion(const std::string &value);

    /**
     * Sets the program version.
     * @param major Major version.
     * @param minor Minor version.
     * @param subminor Subminor version.
     * @param subText Subtext.
     */
    void setVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string & subText);

    /**
     * Returns the daemon name.
     * @return Daemon name.
     */
    std::string getDaemonName() const;

    /**
     * Sets the daemon name.
     * @param value Daemon name.
     */
    void setDaemonName(const std::string &value);


    std::string m_softwareLicense; /**< Program license. */
    std::string m_softwareDescription; /**< Program description. */
    std::string m_programName; /**< Program name. */

protected:
    std::string m_daemonName; /**< Program name. */

    std::list<Author> m_authors; /**< List of authors. */
    std::string m_version; /**< Program version. */
};

}}}

#endif // PROGRAMVALUES_H
