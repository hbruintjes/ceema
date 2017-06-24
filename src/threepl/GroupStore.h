//
// Created by hbruintjes on 17/03/17.
//

#ifndef CEEMA_GROUPSTORE_H
#define CEEMA_GROUPSTORE_H

#include <protocol/data/Group.h>

#include <libpurple/blist.h>
#include <encoding/hex.h>

class ThreeplGroup {
    ceema::client_id m_owner;
    ceema::group_id m_groupid;
    ceema::group_uid m_groupuid;
    std::string m_name;
    std::vector<ceema::client_id> m_members;
    //TODO: icon path

    // (Unique) ID of the group, can be used as chat ID
    int m_id;

public:
    /**
     * Create group from received data
     * @param owner
     * @param id
     */
    ThreeplGroup(ceema::client_id owner, ceema::group_id gid, int id) :
            m_owner(owner), m_groupid(gid), m_groupuid(ceema::make_group_uid(m_owner, m_groupid)),
            m_members{owner}, m_id(id)
    {
        m_name = ceema::hex_encode(gid) + " owned by " + owner.toString();
    }

    std::vector<ceema::client_id> const& members() const {
        return m_members;
    }

    bool add_member(ceema::client_id const& member) {
        for(auto& cid: m_members) {
            if (cid == member) {
                return false;
            }
        }
        m_members.emplace_back(member);
        return true;
    }

    bool remove_member(ceema::client_id const& member) {
        for(auto iter = m_members.begin(); iter != m_members.end(); ++iter) {
            if (*iter == member) {
                m_members.erase(iter);
                return true;
            }
        }
        return false;
    }

    void set_members(std::vector<ceema::client_id> const& members) {
        m_members = members;
    }


    std::string const& name() const {
        return m_name;
    }

    void set_name(std::string name) {
        if (name.size()) {
            m_name = std::move(name);
        } else {
            m_name = ceema::hex_encode(m_groupid) + " owned by " + m_owner.toString();
        }
    }

    ceema::client_id owner() const {
        return m_owner;
    }

    ceema::group_uid const& uid() const {
        return m_groupuid;
    }

    ceema::group_id const& gid() const {
        return m_groupid;
    }

    int id() const {
        return m_id;
    }

    std::string chat_name() const {
        return ceema::hex_encode(m_groupid) + "@" + m_owner.toString();
    }
};


class GroupStore {
    // TODO: should be map, but no hash method yet
    std::vector<ThreeplGroup> m_groups;

public:
    ThreeplGroup* find_group(ceema::group_uid uid, bool add_if_new = false) {
        return find_group(uid.cid(), uid.gid(), add_if_new);
    }

    ThreeplGroup* find_group(ceema::client_id owner, ceema::group_id gid, bool add_if_new = false) {
        for(auto& group: m_groups) {
            if (group.gid() == gid && group.owner() == owner) {
                return &group;
            }
        }
        if (add_if_new) {
            return add_group(owner, gid);
        } else {
            return nullptr;
        }
    }

    ThreeplGroup* find_group(int id) {
        for(auto& group: m_groups) {
            if (group.id() == id) {
                return &group;
            }
        }
        return nullptr;
    }

    ThreeplGroup* find_or_create(GHashTable* components);

    ThreeplGroup* add_group(ceema::client_id owner, ceema::group_id id) {
        m_groups.emplace_back(owner, id, m_groups.size() + 1);
        return  &m_groups.back();
    }

    ThreeplGroup* add_group(ceema::group_uid uid) {
        m_groups.emplace_back(uid.cid(), uid.gid(), m_groups.size() + 1);
        return  &m_groups.back();
    }

    PurpleChat* find_chat(PurpleAccount* account, ThreeplGroup const& group) const;
    void update_chat(PurpleAccount* account, ThreeplGroup const& group) const;
    void update_chats(PurpleAccount* account) const;
    void load_chats(PurpleAccount* account);
};


#endif //CEEMA_GROUPSTORE_H
