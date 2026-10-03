namespace workphone
{
    inline bool Settings::hasValue( const String &key ) const
    {
        return m_values.find( key ) != m_values.end();
    }

    inline bool Settings::isValid() const
    {
        return m_isValid;
    }

    inline const Array<Settings::ValidationIssue> &Settings::getValidationIssues() const
    {
        return m_validationIssues;
    }
}  // namespace workphone
